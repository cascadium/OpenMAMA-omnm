/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Frank Quinn (http://fquinner.github.io)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*=========================================================================
  =                             Includes                                  =
  =========================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <mama/mama.h>
#include <mama/price.h>

#include "omnmmsgpayloadfunctions.h"
#include "omnmmsgpayloadimpl.h"
#include <wombat/strutils.h>
#include <wombat/memnode.h>
#include <stddef.h>
#include <mama/integration/types.h>
#include <mama/integration/mama.h>
#include <mama/integration/msg.h>
#include <mama/integration/msgfield.h>
#include "Payload.h"
#include "Iterator.h"

/*=========================================================================
  =                              Macros                                   =
  =========================================================================*/
#define        FIELD_TYPE_WIDTH        1
#define        FID_WIDTH               2
#define        LENGTH_WIDTH            4
#define        DEFAULT_PAYLOAD_SIZE    200
#define        MAMA_PAYLOAD_ID_OMNM    'O'
#define        OMNM_PROTOCOL_VERSION   1


#define ADD_SCALAR_FIELD(MSG,NAME,FID,VALUE,TYPE)                              \
do                                                                             \
{                                                                              \
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) MSG;                            \
    if (NULL == impl) return MAMA_STATUS_NULL_ARG;                             \
    return impl->addField (TYPE,                                               \
                           NAME,                                               \
                           FID,                                                \
                           (uint8_t*)&VALUE,                                   \
                           sizeof(VALUE));                                     \
} while (0)

#define APPLY_SCALAR_FIELD(SUFFIX,MAMATYPE)                                    \
do                                                                             \
{                                                                              \
    MAMATYPE result;                                                           \
    omnmmsgFieldPayload_get##SUFFIX (field, &result);                          \
    omnmmsgPayload_update##SUFFIX   (dest, name, fid, result);                 \
} while (0)

#define UPDATE_SCALAR_FIELD(MSG,NAME,FID,VALUE,TYPE)                           \
do                                                                             \
{                                                                              \
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) MSG;                            \
    if (NULL == impl) return MAMA_STATUS_NULL_ARG;                             \
    return impl->updateField (TYPE,                                            \
                              NAME,                                            \
                              FID,                                             \
                              (uint8_t*)&VALUE,                                \
                              sizeof(VALUE));                                  \
} while (0)

#define GET_SCALAR_VECTOR(MSG,NAME,FID,RESULT,SIZE,TYPE)                       \
do                                                                             \
{                                                                              \
    mama_status status = MAMA_STATUS_OK;                                       \
    omnmFieldImpl field;                                                       \
                                                                               \
    if (NULL == MSG || NULL == RESULT || NULL == SIZE)                         \
        return MAMA_STATUS_NULL_ARG;                                           \
                                                                               \
    /* populate field with result */                                           \
    status = ((OmnmPayloadImpl *) MSG)->getField(NAME, FID, field);            \
                                                                               \
    if (MAMA_STATUS_OK == status)                                              \
    {                                                                          \
        *size = field.mSize / (sizeof(TYPE));                                  \
        *result = (TYPE*) field.mData;                                         \
    }                                                                          \
                                                                               \
    return status;                                                             \
} while (0)

#define ADD_SCALAR_VECTOR(MSG,NAME,FID,VALUE,SIZE,TYPE,MAMATYPE)               \
do                                                                             \
{                                                                              \
    if (NULL == msg || NULL == VALUE || (NULL == NAME && 0 == FID))            \
        return MAMA_STATUS_NULL_ARG;                                           \
                                                                               \
    return ((OmnmPayloadImpl*) msg)->addField (MAMATYPE,                       \
                                               name,                           \
                                               fid,                            \
                                               (uint8_t*)value,                \
                                               size * sizeof(TYPE));           \
} while (0)

#define UPDATE_SCALAR_VECTOR(MSG,NAME,FID,VALUE,SIZE,TYPE,MAMATYPE)            \
do                                                                             \
{                                                                              \
    mama_status status = MAMA_STATUS_OK;                                       \
    omnmFieldImpl field;                                                       \
                                                                               \
    if (NULL == MSG || NULL == VALUE || (NULL == NAME && 0 == FID))            \
        return MAMA_STATUS_NULL_ARG;                                           \
                                                                               \
    /* populate field with current value */                                    \
    status = ((OmnmPayloadImpl *) MSG)->getField(NAME, FID, field);            \
    if (MAMA_STATUS_NOT_FOUND == status)                                       \
        /* Note this macro function will return */                             \
        ADD_SCALAR_VECTOR(MSG, NAME, FID, VALUE, SIZE, TYPE, MAMATYPE);        \
    else                                                                       \
        ((OmnmPayloadImpl*) MSG)->updateField (MAMATYPE,                       \
                                               field,                          \
                                               (uint8_t*)VALUE,                \
                                               SIZE*(sizeof(TYPE)));           \
                                                                               \
    return status;                                                             \
} while (0)

#define UPDATE_VECTOR_FIELD(SUFFIX,MAMATYPE)                                   \
do                                                                             \
{                                                                              \
    const MAMATYPE * result = NULL;                                            \
    mama_size_t size = 0;                                                      \
    omnmmsgFieldPayload_getVector##SUFFIX (field, &result, &size);             \
    omnmmsgPayload_updateVector##SUFFIX   (dest, name, fid, result, size);     \
} while (0)

/*=========================================================================
  =                  Private implementation prototypes                    =
  =========================================================================*/

OmnmPayloadImpl::OmnmPayloadImpl() : mPayloadBuffer(nullptr),
                                     mPayloadBufferSize(0),
                                     mPayloadBufferTail(0),
                                     mField(), /* Inline struct member */
                                     mHeader(),
                                     mParent(nullptr),
                                     mExtenderClosure(nullptr)
{
    mPayloadBufferSize           = DEFAULT_PAYLOAD_SIZE;
    mPayloadBuffer               = (uint8_t*) calloc (mPayloadBufferSize, 1);

    // Initialize with defaults
    clear();
}

OmnmPayloadImpl::~OmnmPayloadImpl()
{
    if (NULL != mPayloadBuffer)
    {
        free (mPayloadBuffer);
    }
    omnmmsgFieldPayloadImpl_cleanup(&mField);
}

bool
OmnmPayloadImpl::isFieldTypeSized (mamaFieldType   type)
{
    switch(type)
    {
        case MAMA_FIELD_TYPE_BOOL:
        case MAMA_FIELD_TYPE_CHAR:
        case MAMA_FIELD_TYPE_I8:
        case MAMA_FIELD_TYPE_U8:
        case MAMA_FIELD_TYPE_I16:
        case MAMA_FIELD_TYPE_U16:
        case MAMA_FIELD_TYPE_I32:
        case MAMA_FIELD_TYPE_U32:
        case MAMA_FIELD_TYPE_F32:
        case MAMA_FIELD_TYPE_QUANTITY:
        case MAMA_FIELD_TYPE_I64:
        case MAMA_FIELD_TYPE_U64:
        case MAMA_FIELD_TYPE_F64:
        case MAMA_FIELD_TYPE_TIME:
        case MAMA_FIELD_TYPE_PRICE:
        case MAMA_FIELD_TYPE_STRING:
            return false;
            break;
        case MAMA_FIELD_TYPE_MSG:
        case MAMA_FIELD_TYPE_OPAQUE:
        case MAMA_FIELD_TYPE_COLLECTION:
        case MAMA_FIELD_TYPE_VECTOR_BOOL:
        case MAMA_FIELD_TYPE_VECTOR_CHAR:
        case MAMA_FIELD_TYPE_VECTOR_F32:
        case MAMA_FIELD_TYPE_VECTOR_F64:
        case MAMA_FIELD_TYPE_VECTOR_I16:
        case MAMA_FIELD_TYPE_VECTOR_I32:
        case MAMA_FIELD_TYPE_VECTOR_I64:
        case MAMA_FIELD_TYPE_VECTOR_I8:
        case MAMA_FIELD_TYPE_VECTOR_MSG:
        case MAMA_FIELD_TYPE_VECTOR_PRICE:
        case MAMA_FIELD_TYPE_VECTOR_STRING:
        case MAMA_FIELD_TYPE_VECTOR_TIME:
        case MAMA_FIELD_TYPE_VECTOR_U16:
        case MAMA_FIELD_TYPE_VECTOR_U32:
        case MAMA_FIELD_TYPE_VECTOR_U64:
        case MAMA_FIELD_TYPE_VECTOR_U8:
        case MAMA_FIELD_TYPE_UNKNOWN:
        default:
            return true;
            break;
    }
}

bool
OmnmPayloadImpl::isFieldTypeFixedWidth (mamaFieldType type)
{
    switch(type)
    {
        case MAMA_FIELD_TYPE_BOOL:
        case MAMA_FIELD_TYPE_CHAR:
        case MAMA_FIELD_TYPE_I8:
        case MAMA_FIELD_TYPE_U8:
        case MAMA_FIELD_TYPE_I16:
        case MAMA_FIELD_TYPE_U16:
        case MAMA_FIELD_TYPE_I32:
        case MAMA_FIELD_TYPE_U32:
        case MAMA_FIELD_TYPE_F32:
        case MAMA_FIELD_TYPE_QUANTITY:
        case MAMA_FIELD_TYPE_I64:
        case MAMA_FIELD_TYPE_U64:
        case MAMA_FIELD_TYPE_F64:
            return true;
            break;
        case MAMA_FIELD_TYPE_TIME:
        case MAMA_FIELD_TYPE_PRICE:
        case MAMA_FIELD_TYPE_STRING:
        case MAMA_FIELD_TYPE_MSG:
        case MAMA_FIELD_TYPE_OPAQUE:
        case MAMA_FIELD_TYPE_COLLECTION:
        case MAMA_FIELD_TYPE_VECTOR_BOOL:
        case MAMA_FIELD_TYPE_VECTOR_CHAR:
        case MAMA_FIELD_TYPE_VECTOR_F32:
        case MAMA_FIELD_TYPE_VECTOR_F64:
        case MAMA_FIELD_TYPE_VECTOR_I16:
        case MAMA_FIELD_TYPE_VECTOR_I32:
        case MAMA_FIELD_TYPE_VECTOR_I64:
        case MAMA_FIELD_TYPE_VECTOR_I8:
        case MAMA_FIELD_TYPE_VECTOR_MSG:
        case MAMA_FIELD_TYPE_VECTOR_PRICE:
        case MAMA_FIELD_TYPE_VECTOR_STRING:
        case MAMA_FIELD_TYPE_VECTOR_TIME:
        case MAMA_FIELD_TYPE_VECTOR_U16:
        case MAMA_FIELD_TYPE_VECTOR_U32:
        case MAMA_FIELD_TYPE_VECTOR_U64:
        case MAMA_FIELD_TYPE_VECTOR_U8:
        case MAMA_FIELD_TYPE_UNKNOWN:
        default:
            return false;
            break;
    }
}

bool
OmnmPayloadImpl::areFieldTypesCastable (mamaFieldType from, mamaFieldType to)
{
    if (OmnmPayloadImpl::isFieldTypeFixedWidth (from) && OmnmPayloadImpl::isFieldTypeFixedWidth (to))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void
OmnmPayloadImpl::convertOmnmDateTimeToMamaDateTime (omnmDateTime* from, mamaDateTime to)
{
    mamaDateTime_setWithHints(to,
                              (mama_u32_t)from->mSeconds,
                              from->mNanoseconds / 1000,
                              (mamaDateTimePrecision)from->mPrecision,
                              (mamaDateTimeHints)from->mHints);
}

void
OmnmPayloadImpl::convertMamaDateTimeToOmnmDateTime (mamaDateTime from, omnmDateTime* to)
{
    mamaDateTimeHints hints;
    mamaDateTimePrecision precision;
    mama_u32_t seconds = 0, microseconds = 0;

    memset(to, 0, sizeof(omnmDateTime));

    mamaDateTime_getWithHints (from, &seconds, &microseconds, &precision, &hints);

    to->mPrecision = (mama_u8_t)precision;
    to->mNanoseconds = ((mama_u32_t)microseconds) * (mama_u32_t)1000;
    to->mSeconds = (mama_u32_t)seconds;
    to->mHints = (mama_u8_t)hints;
}

void
OmnmPayloadImpl::convertOmnmPriceToMamaPrice (omnmPrice* from, mamaPrice to)
{
    mamaPrice_setWithHints (to, (double)from->mValue, (mamaPriceHints)from->mHints);
}

void
OmnmPayloadImpl::convertMamaPriceToOmnmPrice (mamaPrice from, omnmPrice* to)
{
    double value;
    mamaPricePrecision precision;
    mamaPriceHints hints;

    mamaPrice_getPrecision(from, &precision);
    mamaPrice_getHints(from, &hints);
    mamaPrice_getValue(from, &value);

    memset(to, 0, sizeof(omnmPrice));

    to->mValue = (mama_f64_t)value;
    to->mHints = (mama_u8_t)hints;
}

mama_status
OmnmPayloadImpl::clear()
{
    // Initialize header with defaults
    mHeader.mType                = MAMA_PAYLOAD_ID_OMNM;
    mHeader.mWireFormatVersion   = OMNM_PROTOCOL_VERSION;
    mHeader.mRemainingHeaderSize = sizeof(omnmHeader) - sizeof(omnmHeaderV1);

    // Populate header types and move past
    memcpy(mPayloadBuffer, &mHeader, sizeof(omnmHeader));

    // Make sure tail is set to after the 'type' byte
    mPayloadBufferTail = (size_t) getHeaderSize();

    // NULL initialize the buffer after the first byte
    memset ((void*)(mPayloadBuffer + mPayloadBufferTail), 0, mPayloadBufferSize - mPayloadBufferTail);

    return MAMA_STATUS_OK;
}

uint16_t
OmnmPayloadImpl::getHeaderSize()
{
    return (uint16_t)(sizeof(omnmHeaderV1) + mHeader.mRemainingHeaderSize);
}

mama_status
OmnmPayloadImpl::findFieldInBuffer (const char* name, mama_fid_t fid, omnmFieldImpl& field)
{
    // Initialize the iterator struct on the stack for speed
    omnmIterImpl   iter;
    omnmFieldImpl* fieldCandidate;

    // NULL initialize any provided field
    //memset (&field, 0, sizeof(field));

    // This is really just for type casting so we can easily use bridge
    // iterator methods directly
    msgPayloadIter iterOpaque = (msgPayloadIter)&iter;

    // Initialize the iterator for this message
    omnmmsgPayloadIterImpl_init (&iter, this);

    // Iterate over all fields until
    while (NULL != (fieldCandidate = (omnmFieldImpl*)omnmmsgPayloadIter_next (iterOpaque, NULL, this)))
    {
        // Fid match - found the field
        if ( ((0 != fid) && (fid == fieldCandidate->mFid))
          || ((name != NULL) && (fieldCandidate->mName != NULL) && (0 == strcmp (name, fieldCandidate->mName))) )
        {
            // Copy the field's data to the provided onmnFieldImpl
            //memcpy (&field, fieldCandidate, sizeof(omnmFieldImpl));
            /* Itemize as field also holds accumulative data structures */
            field.mFieldType = fieldCandidate->mFieldType;
            field.mFid       = fieldCandidate->mFid;
            field.mName      = fieldCandidate->mName;
            field.mSize      = fieldCandidate->mSize;
            field.mData      = fieldCandidate->mData;
            field.mParent    = fieldCandidate->mParent;

            return MAMA_STATUS_OK;
        }
    }

    // If we got this far, no match has been found
    return MAMA_STATUS_NOT_FOUND;
}

mama_status
OmnmPayloadImpl::addField (mamaFieldType type, const char* name, mama_fid_t fid,
        uint8_t* buffer, size_t bufferLen)
{
    if (bufferLen > UINT32_MAX) return MAMA_STATUS_INVALID_ARG;

    // We will insert at the tail
    uint8_t* insertPoint = mPayloadBuffer + mPayloadBufferTail;

    // New tail will be comprised of type, fid, field
    size_t newTailOffset = mPayloadBufferTail + FIELD_TYPE_WIDTH + FID_WIDTH +
            strlenEx(name) + 1 + bufferLen;

    if (NULL == buffer || 0 == bufferLen || (NULL == name && 0 == fid))
    {
        return MAMA_STATUS_NULL_ARG;
    }

    // If a variable width field, buffer will also contain a size
    if (isFieldTypeSized(type))
    {
        newTailOffset += sizeof(mama_u32_t);
    }

    VALIDATE_NAME_FID(name, fid);

    // Ensure the buffer is big enough for this
    allocateBufferMemory ((void**)&mPayloadBuffer,
                          &mPayloadBufferSize,
                          newTailOffset);

    // Will insert at wherever the current tail is
    insertPoint = mPayloadBuffer + mPayloadBufferTail;

    // Update the field type
    *insertPoint = (uint8_t)type;
    insertPoint += sizeof(uint8_t);

    // Update the fid
    *((uint16_t*)insertPoint) = fid;
    insertPoint += sizeof(uint16_t);

    // Update the name
    if (NULL == name)
    {
        *insertPoint = '\0';
        insertPoint += sizeof(uint8_t);
    }
    else
    {
        // Copy string including terminator
        memcpy ((void*)insertPoint, name, strlen(name) + 1);
        insertPoint += strlen(name) + 1;
    }

    // If a variable width field, buffer will also need copy of size
    if (isFieldTypeSized(type))
    {
        mama_u32_t len = (mama_u32_t) bufferLen;
        memcpy ((void*)insertPoint, (void*)&len, sizeof(len));
        insertPoint += sizeof(mama_u32_t);
    }

    // Copy across the data itself
    memcpy ((void*)insertPoint, (void*)buffer, bufferLen);

    // Update the tail position
    mPayloadBufferTail = newTailOffset;

    // If name and fid is null, add 'bare'
    return MAMA_STATUS_OK;
}

mama_status
OmnmPayloadImpl::updateField (mamaFieldType type, const char* name,
        mama_fid_t fid, uint8_t* buffer, size_t bufferLen)
{
    omnmFieldImpl field;

    if (NULL == buffer || (NULL == name && 0 == fid))
    {
        return MAMA_STATUS_NULL_ARG;
    }

    mama_status status = findFieldInBuffer (name, fid, field);

    // Add field if it can't be found
    if (MAMA_STATUS_OK != status)
    {
        return addField (type, name, fid, buffer, bufferLen);
    }

    if (field.mFieldType != type &&
        false == OmnmPayloadImpl::areFieldTypesCastable(field.mFieldType, type))
    {
        return MAMA_STATUS_WRONG_FIELD_TYPE;
    }

    // If the field exists, try updating it
    return updateField (type, field, buffer, bufferLen);
}

mama_status
OmnmPayloadImpl::updateField (mamaFieldType type, omnmFieldImpl& field,
        uint8_t* buffer, size_t bufferLen)
{
    if (NULL == buffer)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    if (field.mFieldType != type &&
        false == OmnmPayloadImpl::areFieldTypesCastable(field.mFieldType, type))
    {
        return MAMA_STATUS_WRONG_FIELD_TYPE;
    }

    // If buffer needs to expand or shrink
    if (bufferLen != field.mSize)
    {
        int32_t delta = (int32_t)(bufferLen - field.mSize);

        // This will be the new offset of next field after the move
        size_t nextByteOffset = ((uint8_t*)field.mData - mPayloadBuffer) +
                field.mSize;

        // Increase the buffer *if necessary* (only)
        if (delta > 0)
        {
            const uint8_t* payloadBufferPrev = mPayloadBuffer;

            // Increase buffer memory
            allocateBufferMemory ((void**)&mPayloadBuffer,
                                  &mPayloadBufferSize,
                                  mPayloadBufferSize + delta);

            // If the allocateBufferMemory (realloc) has actually moved the underlying buffer
            if(payloadBufferPrev != mPayloadBuffer)
            {
              // buffers have moved so re-apply offsets
              field.mData = mPayloadBuffer + ((uint8_t*)field.mData - payloadBufferPrev);
              field.mName = (const char*)mPayloadBuffer + ((uint8_t*)field.mName - payloadBufferPrev);
            }
        }

        // If the field has changed in size (increased or decreased), we need to move memory
        if (delta != 0)
        {
            // This will point to the target location of the first field after inserted field
            uint8_t* origin = mPayloadBuffer + nextByteOffset;
            // This will correspond to the number of remaining bytes after the updated field
            uint32_t size = mPayloadBufferTail - nextByteOffset;
            // Finally move the memory across
            memmove ((void*)(origin + delta), origin, size);
        }
        mPayloadBufferTail += delta;
    }

    if (isFieldTypeSized(field.mFieldType))
    {
        uint32_t dataSize = (uint32_t) bufferLen;
        memcpy (((uint8_t*)field.mData - sizeof(uint32_t)), &dataSize, sizeof(uint32_t));
    }
    memcpy ((void*)field.mData, (void*)buffer, bufferLen);
    return MAMA_STATUS_OK;
}


mama_status
OmnmPayloadImpl::updateSubMsg (msgPayload msg, const char* name, mama_fid_t fid, const msgPayload value)
{
    const void* buffer = NULL;
    mama_size_t bufferLen = 0;
    omnmmsgPayload_serialize (value, &buffer, &bufferLen);

    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_MSG,
                                                  name,
                                                  fid,
                                                  (uint8_t*)buffer,
                                                  bufferLen);
}



/*=========================================================================
  =                   Public interface functions                          =
  =========================================================================*/

mama_status
omnmmsgPayload_init (mamaPayloadBridge bridge, char* identifier)
{
    *identifier = (char)MAMA_PAYLOAD_ID_OMNM;

    /* Will set the bridge's compile time MAMA version */
    MAMA_SET_BRIDGE_COMPILE_TIME_VERSION("omnmmsg");

    return MAMA_STATUS_OK;
}

MAMAIgnoreDeprecatedOpen
mamaPayloadType
omnmmsgPayload_getType (void)
{
    return (mamaPayloadType)MAMA_PAYLOAD_ID_OMNM;
}
MAMAIgnoreDeprecatedClose

mama_status
omnmmsgPayload_create (msgPayload* msg)
{
    if (NULL == msg) return MAMA_STATUS_NULL_ARG;

    OmnmPayloadImpl* impl = new OmnmPayloadImpl();

    *msg = (msgPayload) impl;

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_destroy (msgPayload msg)
{
    if (NULL == msg) return MAMA_STATUS_NULL_ARG;

    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;

    delete impl;

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_createForTemplate (msgPayload*        msg,
                                  mamaPayloadBridge  bridge,
                                  mama_u32_t         templateId)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_copy (const msgPayload    msg,
                     msgPayload*         copy)
{
    OmnmPayloadImpl*  impl        = (OmnmPayloadImpl*) msg;
    mama_status       status      = MAMA_STATUS_OK;

    if (NULL == msg || NULL == copy)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    /*
     * Create destination message object if the user hasn't already created
     * one
     */
    if (NULL == *copy)
    {
        status = omnmmsgPayload_create (copy);
        if (MAMA_STATUS_OK != status)
        {
            return status;
        }
    }

    return omnmmsgPayload_unSerialize ((OmnmPayloadImpl*) *copy,
                                       (const void**)impl->mPayloadBuffer,
                                       impl->mPayloadBufferTail);
}

mama_status
omnmmsgPayload_clear (msgPayload msg)
{
    if (NULL == msg) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl*) msg)->clear();
}

mama_status
omnmmsgPayload_setParent (msgPayload    msg,
                          const mamaMsg parent)
{
    if (NULL == msg) return MAMA_STATUS_NULL_ARG;
    ((OmnmPayloadImpl*) msg)->mParent = (mamaMsg) parent;
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_getByteSize (msgPayload    msg,
                            mama_size_t*  size)
{
    if (NULL == msg) return MAMA_STATUS_NULL_ARG;
    *size = ((OmnmPayloadImpl*) msg)->mPayloadBufferTail;
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_getNumFields (const msgPayload    msg,
                             mama_size_t*        numFields)
{
    omnmIterImpl        iter;
    OmnmPayloadImpl*    impl            = (OmnmPayloadImpl*) msg;
    mama_size_t         count           = 0;

    if (NULL == msg || NULL == numFields) return MAMA_STATUS_NULL_ARG;

    // This is really just for type casting so we can easily use bridge
    // iterator methods directly
    msgPayloadIter iterOpaque = (msgPayloadIter)&iter;

    // Initialize the iterator for this message
    omnmmsgPayloadIterImpl_init (&iter, impl);

    // Iterate over all fields
    while (NULL != omnmmsgPayloadIter_next(iterOpaque, NULL, msg))
    {
        count++;
    }
    *numFields = count;

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_getSendSubject (const msgPayload    msg,
                               const char**        subject)
{
    /*
     * Not implemented for now. Used by self describing messages and the qpid
     * payload is not self describing. We could add this capability later.
     */
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

const char*
omnmmsgPayload_toString (const msgPayload msg)
{
    omnmIterImpl        iter;
    OmnmPayloadImpl*    impl            = (OmnmPayloadImpl*) msg;
    msgFieldPayload     fieldPayload    = NULL;
    mama_size_t         numFields       = 0;
    mama_size_t         charIdx         = 0; /* Opening Brace */
    char                part[1024];

    if (NULL == msg) return NULL;

    omnmmsgPayload_getNumFields (msg, &numFields);

    // This is really just for type casting so we can easily use bridge
    // iterator methods directly
    msgPayloadIter iterOpaque = (msgPayloadIter)&iter;

    // Initialize the iterator for this message
    omnmmsgPayloadIterImpl_init (&iter, impl);

    if (0 != allocateBufferMemory ((void**) &impl->mField.mBuffer,
                                   &impl->mField.mBufferLen,
                                   sizeof(part)))
    {
        return NULL;
    }

    charIdx += sprintf ((char*)impl->mField.mBuffer + charIdx, "{");

    // Iterate over all fields
    while (NULL != (fieldPayload = omnmmsgPayloadIter_next(iterOpaque, NULL, msg)))
    {
        // TODO: actually build the string
        mama_fid_t      fid           = 0;
        const char*     fname         = NULL;
        mama_size_t     bytesInString = 0;

        omnmmsgFieldPayload_getAsString (fieldPayload, impl, part, sizeof(part));
        omnmmsgFieldPayload_getFid (fieldPayload, NULL, NULL, &fid);
        omnmmsgFieldPayload_getName (fieldPayload, NULL, NULL, &fname);

        bytesInString = strlenEx(fname) + strlen(part) + 10;

        if (0 != allocateBufferMemory ((void**) &impl->mField.mBuffer,
                                       &impl->mField.mBufferLen,
                                       bytesInString + charIdx))
        {
            return NULL;
        }

        if (fid == 0)
        {
           charIdx += sprintf ((char*)impl->mField.mBuffer + charIdx,
                               "%s=%s",
                               fname ? fname : "",
                               part);
        }
        else
        {
           charIdx += sprintf ((char*)impl->mField.mBuffer + charIdx,
                               "%s[%u]=%s",
                               fname ? fname : "",
                               fid,
                               part);
        }

        if (omnmmsgPayloadIter_hasNext (iterOpaque, msg))
        {
            charIdx += sprintf((char*)impl->mField.mBuffer + charIdx, ",");
        }
    }

    sprintf((char*)impl->mField.mBuffer + charIdx, "}");

    return (const char*) impl->mField.mBuffer;
}

mama_status
omnmmsgPayload_iterateFields (const msgPayload    msg,
                              const mamaMsg       parent,
                              mamaMsgField        field,
                              mamaMsgIteratorCb   cb,
                              void*               closure)
{
    omnmIterImpl        iter;
    OmnmPayloadImpl*    impl            = (OmnmPayloadImpl*) msg;
    msgFieldPayload     fieldPayload    = NULL;

    if (NULL == msg || NULL == parent || NULL == cb || NULL == field)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    // This is really just for type casting so we can easily use bridge
    // iterator methods directly
    msgPayloadIter iterOpaque = (msgPayloadIter)&iter;

    // Initialize the iterator for this message
    omnmmsgPayloadIterImpl_init (&iter, impl);

    while (NULL != (fieldPayload = omnmmsgPayloadIter_next(iterOpaque, NULL, msg)))
    {
        mamaMsgFieldImpl_setPayload (field, fieldPayload);
        cb (parent, field, closure);
    }

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_serialize (const msgPayload  msg,
                          const void**      buffer,
                          mama_size_t*      bufferLength)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;
    if (NULL == msg || NULL == buffer || NULL == bufferLength)
        return MAMA_STATUS_NULL_ARG;

    *buffer = impl->mPayloadBuffer;
    *bufferLength = impl->mPayloadBufferTail;
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_unSerialize (const msgPayload    msg,
                            const void**        buffer,
                            mama_size_t         bufferLength)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;
    omnmHeader       header;

    if (NULL == msg || NULL == buffer || 0 == bufferLength)
        return MAMA_STATUS_NULL_ARG;

    // New buffer incoming - check header for version compatibility
    memcpy(&header, buffer, sizeof(header));
    if (header.mWireFormatVersion > OMNM_PROTOCOL_VERSION)
    {
        return MAMA_STATUS_WRONG_FIELD_TYPE;
    }

    // Wipe current buffer
    memset (impl->mPayloadBuffer, 0, impl->mPayloadBufferSize);

    // Ensure buffer is big enough to hold
    if (0 != allocateBufferMemory ((void**) &impl->mPayloadBuffer,
                                   &impl->mPayloadBufferSize,
                                   bufferLength))
    {
        return MAMA_STATUS_NOMEM;
    }

    // Do not attempt self copy
    if (impl->mPayloadBuffer != (void*)buffer)
    {
        memcpy (impl->mPayloadBuffer, (void*)buffer, bufferLength);
    }

    // Parse the rest of the header for initialization
    impl->mHeader.mWireFormatVersion = header.mWireFormatVersion;
    impl->mHeader.mRemainingHeaderSize = header.mRemainingHeaderSize;

    // Move tail to end of buffer
    impl->mPayloadBufferTail = bufferLength;

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_getByteBuffer (const msgPayload  msg,
                              const void**      buffer,
                              mama_size_t*      bufferLength)
{
    if (NULL == msg || NULL == buffer || NULL == bufferLength) return MAMA_STATUS_NULL_ARG;
    return omnmmsgPayload_serialize (msg, buffer, bufferLength);
}

/*
 * Note this function sets an underlying implementation handle - not
 * necessarily a serialized version of the message.
 */
mama_status
omnmmsgPayload_setByteBuffer (const msgPayload    msg,
                              mamaPayloadBridge   bridge,
                              const void*         buffer,
                              mama_size_t         bufferLength)
{
    if (NULL == msg || NULL == buffer) return MAMA_STATUS_NULL_ARG;
    if (0 == bufferLength) return MAMA_STATUS_INVALID_ARG;
    return omnmmsgPayload_unSerialize (msg, (const void**)buffer, bufferLength);
}

mama_status
omnmmsgPayload_createFromByteBuffer (msgPayload*         msg,
                                     mamaPayloadBridge   bridge,
                                     const void*         buffer,
                                     mama_size_t         bufferLength)
{
    if (NULL == msg || NULL == buffer) return MAMA_STATUS_NULL_ARG;
    if (0 == bufferLength) return MAMA_STATUS_INVALID_ARG;
    omnmmsgPayload_create (msg);
    return omnmmsgPayload_unSerialize (*msg, (const void**)buffer, bufferLength);
}

mama_status
omnmmsgPayload_apply (msgPayload          dest,
                      const msgPayload    src)
{
    OmnmPayloadImpl* msg    = (OmnmPayloadImpl*) src;
    mamaFieldType   type    = MAMA_FIELD_TYPE_UNKNOWN;
    const char*     name    = NULL;
    uint16_t        fid     = 0;

    if (NULL == dest || NULL == src) return MAMA_STATUS_NULL_ARG;

    // Initialize the iterator struct on the stack
    omnmIterImpl iter;
    omnmFieldImpl* field;

    // This is really just for type casting so we can easily use bridge
    // iterator methods directly
    msgPayloadIter iterOpaque = (msgPayloadIter)&iter;

    // Initialize the iterator for this message
    omnmmsgPayloadIterImpl_init (&iter, msg);

    while (NULL != (field = (omnmFieldImpl*)omnmmsgPayloadIter_next (iterOpaque, NULL, msg)))
    {
        omnmmsgFieldPayload_getType (field, &type);
        omnmmsgFieldPayload_getName (field, NULL, NULL, &name);
        omnmmsgFieldPayload_getFid  (field, NULL, NULL, &fid);
        switch (field->mFieldType) {
        case MAMA_FIELD_TYPE_BOOL:
        {
            APPLY_SCALAR_FIELD (Bool, mama_bool_t);
            break;
        }
        case MAMA_FIELD_TYPE_CHAR:
        {
            APPLY_SCALAR_FIELD (Char, char);
            break;
        }
        case MAMA_FIELD_TYPE_U8:
        {
            APPLY_SCALAR_FIELD (U8, mama_u8_t);
            break;
        }
        case MAMA_FIELD_TYPE_I8:
        {
            APPLY_SCALAR_FIELD (I8, mama_i8_t);
            break;
        }
        case MAMA_FIELD_TYPE_U16:
        {
            APPLY_SCALAR_FIELD (U16, mama_u16_t);
            break;
        }
        case MAMA_FIELD_TYPE_I16:
        {
            APPLY_SCALAR_FIELD (I16, mama_i16_t);
            break;
        }
        case MAMA_FIELD_TYPE_U32:
        {
            APPLY_SCALAR_FIELD (U32, mama_u32_t);
            break;
        }
        case MAMA_FIELD_TYPE_I32:
        {
            APPLY_SCALAR_FIELD (I32, mama_i32_t);
            break;
        }
        case MAMA_FIELD_TYPE_U64:
        {
            APPLY_SCALAR_FIELD (U64, mama_u64_t);
            break;
        }
        case MAMA_FIELD_TYPE_I64:
        {
            APPLY_SCALAR_FIELD (I64, mama_i64_t);
            break;
        }
        case MAMA_FIELD_TYPE_F32:
        {
            APPLY_SCALAR_FIELD (F32, mama_f32_t);
            break;
        }
        case MAMA_FIELD_TYPE_F64:
        {
            APPLY_SCALAR_FIELD (F64, mama_f64_t);
            break;
        }
        case MAMA_FIELD_TYPE_STRING:
        {
            const char* result = NULL;

            omnmmsgFieldPayload_getString (field, &result);
            omnmmsgPayload_updateString   (dest, name, fid, result);
            break;
        }
        case MAMA_FIELD_TYPE_MSG:
        {
            /*
             * Slight difference in naming convention for get and update methods
             * so we don't use the define expansion here.
             */
            msgPayload result = NULL;

            omnmmsgFieldPayload_getMsg  (field, &result);
            ((OmnmPayloadImpl*) msg)->updateSubMsg (dest, name, fid, result);
            break;
        }
        case MAMA_FIELD_TYPE_OPAQUE:
        {
            /*
             * Slight difference in function prototypes, so we don't use the
             * define expansion here.
             */
            const void* result  = NULL;
            mama_size_t size    = 0;

            omnmmsgFieldPayload_getOpaque (field, &result, &size);
            omnmmsgPayload_updateOpaque   (dest, name, fid, result, size);
            break;
        }
        case MAMA_FIELD_TYPE_TIME:
        {
            mamaDateTime result = NULL;

            mamaDateTime_create             (&result);
            omnmmsgFieldPayload_getDateTime (field, result);
            omnmmsgPayload_updateDateTime   (dest, name, fid, result);
            mamaDateTime_destroy            (result);

            break;
        }
        case MAMA_FIELD_TYPE_PRICE:
        {
            mamaPrice result = NULL;

            mamaPrice_create             (&result);
            omnmmsgFieldPayload_getPrice (field, result);
            omnmmsgPayload_updatePrice   (dest, name, fid, result);
            mamaPrice_destroy            (result);

            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_U8:
        {
            UPDATE_VECTOR_FIELD (U8, mama_u8_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_BOOL:
        {
            UPDATE_VECTOR_FIELD (Bool, mama_bool_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_CHAR:
        {
            UPDATE_VECTOR_FIELD (Char, char);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_I8:
        {
            UPDATE_VECTOR_FIELD (I8, mama_i8_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_U16:
        {
            UPDATE_VECTOR_FIELD (U16, mama_u16_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_I16:
        {
            UPDATE_VECTOR_FIELD (I16, mama_i16_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_U32:
        {
            UPDATE_VECTOR_FIELD (U32, mama_u32_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_I32:
        {
            UPDATE_VECTOR_FIELD (I32, mama_i32_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_U64:
        {
            UPDATE_VECTOR_FIELD (U64, mama_u64_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_I64:
        {
            UPDATE_VECTOR_FIELD (I64, mama_i64_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_F32:
        {
            UPDATE_VECTOR_FIELD (F32, mama_f32_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_F64:
        {
            UPDATE_VECTOR_FIELD (F64, mama_f64_t);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_STRING:
        {
            UPDATE_VECTOR_FIELD (String, char*);
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_MSG:
        {
            break;
        }
        case MAMA_FIELD_TYPE_VECTOR_PRICE:
        case MAMA_FIELD_TYPE_VECTOR_TIME:
        case MAMA_FIELD_TYPE_QUANTITY:
        case MAMA_FIELD_TYPE_UNKNOWN:
        default:
            break;
        }
    }
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_getNativeMsg (const msgPayload    msg,
                             void**              nativeMsg)
{
    if (NULL == msg || NULL == nativeMsg)
    {
        return MAMA_STATUS_NULL_ARG;
    }
    *nativeMsg = (void*) msg;
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_getFieldAsString (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 char*               buffer,
                                 mama_size_t         len)
{
    omnmFieldImpl targetField;
    memset (&targetField, 0, sizeof(targetField));
    VALIDATE_NON_NULL(msg);
    ((OmnmPayloadImpl*) msg)->getField(name, fid, targetField);
    return omnmmsgFieldPayload_getAsString (&targetField,
                                            msg,
                                            buffer,
                                            len);
}

mama_status
omnmmsgPayload_addBool (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_bool_t value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_BOOL);
}

mama_status
omnmmsgPayload_addChar (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        char        value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_CHAR);
}

mama_status
omnmmsgPayload_addI8   (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_i8_t   value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_I8);
}

mama_status
omnmmsgPayload_addU8   (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_u8_t   value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_U8);
}

mama_status
omnmmsgPayload_addI16  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_i16_t  value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_I16);
}

mama_status
omnmmsgPayload_addU16  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_u16_t  value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_U16);
}

mama_status
omnmmsgPayload_addI32  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_i32_t  value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_I32);
}

mama_status
omnmmsgPayload_addU32  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_u32_t  value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_U32);
}

mama_status
omnmmsgPayload_addI64  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_i64_t  value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_I64);
}

mama_status
omnmmsgPayload_addU64  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_u64_t  value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_U64);
}

mama_status
omnmmsgPayload_addF32  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_f32_t  value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_F32);
}

mama_status
omnmmsgPayload_addF64  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_f64_t  value)
{
    VALIDATE_NON_NULL(msg);
    ADD_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_F64);
}

mama_status
omnmmsgPayload_addString (msgPayload  msg,
                          const char* name,
                          mama_fid_t  fid,
                          const char* str)
{
    VALIDATE_NON_NULL(msg);
    VALIDATE_NON_NULL(str);
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;
    return impl->addField (MAMA_FIELD_TYPE_STRING,
                           name,
                           fid,
                           (uint8_t*)str,
                           strlenEx(str) + 1);
}

mama_status
omnmmsgPayload_addOpaque (msgPayload  msg,
                          const char* name,
                          mama_fid_t  fid,
                          const void* opaque,
                          mama_size_t size)
{
    VALIDATE_NON_NULL(msg);
    VALIDATE_NON_NULL(opaque);
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;
    return impl->addField (MAMA_FIELD_TYPE_OPAQUE,
                           name,
                           fid,
                           (uint8_t*)opaque,
                           size);
}

mama_status
omnmmsgPayload_addDateTime (msgPayload          msg,
                            const char*         name,
                            mama_fid_t          fid,
                            const mamaDateTime  value)
{
    return omnmmsgPayload_updateDateTime(msg, name, fid, value);
}

mama_status
omnmmsgPayload_addPrice (msgPayload      msg,
                         const char*     name,
                         mama_fid_t      fid,
                         const mamaPrice value)
{
    return omnmmsgPayload_updatePrice(msg, name, fid, value);
}

mama_status
omnmmsgPayload_addMsg (msgPayload    msg,
                       const char*   name,
                       mama_fid_t    fid,
                       const mamaMsg value)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;
    const void* buffer = NULL;
    mama_size_t bufferLen = 0;
    mama_status status;
    msgPayload payload;

    VALIDATE_NON_NULL(msg);
    VALIDATE_NON_NULL(value);

    status = mamaMsgImpl_getPayload(value, &payload);
    if (MAMA_STATUS_OK != status)
    {
        return status;
    }

    status = omnmmsgPayload_serialize (payload, &buffer, &bufferLen);
    if (MAMA_STATUS_OK != status)
    {
        return status;
    }

    return impl->addField (MAMA_FIELD_TYPE_MSG,
                           name,
                           fid,
                           (uint8_t*)buffer,
                           bufferLen);
}

mama_status
omnmmsgPayload_addVectorBool (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_bool_t   value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_bool_t, MAMA_FIELD_TYPE_VECTOR_BOOL);
}

mama_status
omnmmsgPayload_addVectorChar (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const char          value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, char, MAMA_FIELD_TYPE_VECTOR_CHAR);
}

mama_status
omnmmsgPayload_addVectorI8   (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_i8_t     value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_i8_t, MAMA_FIELD_TYPE_VECTOR_I8);
}

mama_status
omnmmsgPayload_addVectorU8   (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_u8_t     value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_u8_t, MAMA_FIELD_TYPE_VECTOR_U8);
}

mama_status
omnmmsgPayload_addVectorI16  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_i16_t    value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_i16_t, MAMA_FIELD_TYPE_VECTOR_I16);
}

mama_status
omnmmsgPayload_addVectorU16  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_u16_t    value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_u16_t, MAMA_FIELD_TYPE_VECTOR_U16);
}

mama_status
omnmmsgPayload_addVectorI32  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_i32_t    value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_i32_t, MAMA_FIELD_TYPE_VECTOR_I32);
}

mama_status
omnmmsgPayload_addVectorU32  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_u32_t    value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_u32_t, MAMA_FIELD_TYPE_VECTOR_U32);
}

mama_status
omnmmsgPayload_addVectorI64  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_i64_t    value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_i64_t, MAMA_FIELD_TYPE_VECTOR_I64);
}

mama_status
omnmmsgPayload_addVectorU64  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_u64_t    value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_u64_t, MAMA_FIELD_TYPE_VECTOR_U64);
}

mama_status
omnmmsgPayload_addVectorF32  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_f32_t    value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_f32_t, MAMA_FIELD_TYPE_VECTOR_F32);
}

mama_status
omnmmsgPayload_addVectorF64  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_f64_t    value[],
                              mama_size_t         size)
{
    ADD_SCALAR_VECTOR(msg, name, fid, value, size, mama_f64_t, MAMA_FIELD_TYPE_VECTOR_F64);
}

mama_status
omnmmsgPayload_addVectorString (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char*         value[],
                                mama_size_t         size)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;
    size_t bytesRequired = 0, i = 0;
    VALIDATE_NAME_FID(name, fid);
    VALIDATE_NON_NULL(msg);
    VALIDATE_NON_NULL(value);

    for (i = 0; i < size; i++)
    {
        bytesRequired += strlen(value[i]) + 1;
    }

    // Ensure the buffer is big enough for this
    allocateBufferMemory ((void**)&impl->mField.mBuffer,
                          &impl->mField.mBufferLen,
                          bytesRequired);

    uint8_t* target = (uint8_t*)impl->mField.mBuffer;
    for (i = 0; i < size; i++)
    {
        size_t copyLen = strlen(value[i]) + 1;
        memcpy(target, value[i], copyLen);
        target += copyLen;
    }

    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_VECTOR_STRING,
                                               name,
                                               fid,
                                               (uint8_t*)impl->mField.mBuffer,
                                               impl->mField.mBufferLen);
}

mama_status
omnmmsgPayload_addVectorMsg (msgPayload          msg,
                             const char*         name,
                             mama_fid_t          fid,
                             const mamaMsg       value[],
                             mama_size_t         size)
{
    return omnmmsgPayload_updateVectorMsg (msg,  name, fid, value, size);
}

mama_status
omnmmsgPayload_addVectorDateTime (msgPayload          msg,
                                  const char*         name,
                                  mama_fid_t          fid,
                                  const mamaDateTime  value[],
                                  mama_size_t         size)
{
    return omnmmsgPayload_updateVectorTime(msg, name, fid, value, size);
}

mama_status
omnmmsgPayload_addVectorPrice (msgPayload      msg,
                               const char*     name,
                               mama_fid_t      fid,
                               const mamaPrice value[],
                               mama_size_t     size)
{
    return omnmmsgPayload_updateVectorPrice(msg, name, fid, value, size);
}

mama_status
omnmmsgPayload_updateBool   (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_bool_t  value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_BOOL);
}

mama_status
omnmmsgPayload_updateChar   (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             char         value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_CHAR);
}

mama_status
omnmmsgPayload_updateI8     (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_i8_t    value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_I8);
}

mama_status
omnmmsgPayload_updateU8     (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_u8_t    value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_U8);
}

mama_status
omnmmsgPayload_updateI16    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_i16_t   value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_I16);
}

mama_status
omnmmsgPayload_updateU16    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_u16_t   value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_U16);
}

mama_status
omnmmsgPayload_updateI32    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_i32_t   value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_I32);
}

mama_status
omnmmsgPayload_updateU32    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_u32_t   value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_U32);
}

mama_status
omnmmsgPayload_updateI64    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_i64_t   value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_I64);
}

mama_status
omnmmsgPayload_updateU64    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_u64_t   value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_U64);
}

mama_status
omnmmsgPayload_updateF32    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_f32_t   value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_F32);
}

mama_status
omnmmsgPayload_updateF64    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_f64_t   value)
{
    UPDATE_SCALAR_FIELD(msg, name, fid, value, MAMA_FIELD_TYPE_F64);
}

mama_status
omnmmsgPayload_updateString (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             const char*  str)
{
    if (NULL == msg || NULL == str) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_STRING, name, fid, (uint8_t*)str, strlenEx(str) + 1);
}

mama_status
omnmmsgPayload_updateOpaque (msgPayload          msg,
                             const char*         name,
                             mama_fid_t          fid,
                             const void*         opaque,
                             mama_size_t         size)
{
    if (NULL == msg || NULL == opaque) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_OPAQUE,
                                                  name,
                                                  fid,
                                                  (uint8_t*)opaque,
                                                  size);
}

mama_status
omnmmsgPayload_updateDateTime (msgPayload          msg,
                               const char*         name,
                               mama_fid_t          fid,
                               const mamaDateTime  value)
{
    if (NULL == msg || NULL == value) return MAMA_STATUS_NULL_ARG;

    OmnmPayloadImpl* impl = (OmnmPayloadImpl*)msg;
    omnmDateTime dateTime;

    OmnmPayloadImpl::convertMamaDateTimeToOmnmDateTime (value, &dateTime);

    return impl->updateField (MAMA_FIELD_TYPE_TIME,
                              name,
                              fid,
                              (uint8_t*)&dateTime,
                              sizeof(dateTime));
}

mama_status
omnmmsgPayload_updatePrice (msgPayload          msg,
                            const char*         name,
                            mama_fid_t          fid,
                            const mamaPrice     value)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*)msg;
    omnmPrice price;

    if (NULL == msg || NULL == value) return MAMA_STATUS_NULL_ARG;

    OmnmPayloadImpl::convertMamaPriceToOmnmPrice(value, &price);

    return impl->updateField(MAMA_FIELD_TYPE_PRICE,
        name,
        fid,
        (uint8_t*)&price,
        sizeof(omnmPrice));
}

mama_status
omnmmsgPayload_updateSubMsg (msgPayload          msg,
                             const char*         name,
                             mama_fid_t          fid,
                             const mamaMsg       value)
{
    if (NULL == msg || NULL == value) return MAMA_STATUS_NULL_ARG;

    msgPayload payload = NULL;
    mama_status status = mamaMsgImpl_getPayload (value, &payload);
    if (status != MAMA_STATUS_OK) return status;

    return ((OmnmPayloadImpl*) msg)->updateSubMsg (msg, name, fid, payload);
}

mama_status
omnmmsgPayload_updateVectorMsg (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaMsg       value[],
                                mama_size_t         size)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;
    size_t bytesRequired = 0, i = 0;
    VALIDATE_NAME_FID(name, fid);
    VALIDATE_NON_NULL(msg);
    VALIDATE_NON_NULL(value);

    for (i = 0; i < size; i++)
    {
        mama_size_t msgSize = 0;
        mamaMsg_getByteSize(value[i], &msgSize);
        bytesRequired += msgSize + sizeof(mama_u32_t);
    }

    // Ensure the buffer is big enough for this
    allocateBufferMemory ((void**)&impl->mField.mBuffer,
                          &impl->mField.mBufferLen,
                          bytesRequired);

    uint8_t* target = (uint8_t*)impl->mField.mBuffer;
    for (i = 0; i < size; i++)
    {
        const void* buffer = NULL;
        mama_size_t bufferLen = 0;
        mamaMsg_getByteBuffer(value[i], &buffer, &bufferLen);
        if (bufferLen > UINT32_MAX) return MAMA_STATUS_INVALID_ARG;
        mama_u32_t len = (mama_u32_t) bufferLen;

        /* Put size of each message before each message */
        memcpy((void*)target, &len, sizeof(len));
        target = target + sizeof(mama_u32_t);
        memcpy((void*)target, buffer, len);
        target += bufferLen;
    }

    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_VECTOR_MSG,
                                                  name,
                                                  fid,
                                                  (uint8_t*)impl->mField.mBuffer,
                                                  bytesRequired);
}

mama_status
omnmmsgPayload_updateVectorString (msgPayload   msg,
                                   const char*  name,
                                   mama_fid_t   fid,
                                   const char*  value[],
                                   mama_size_t  size)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;
    size_t bytesRequired = 0, i = 0;
    VALIDATE_NAME_FID(name, fid);
    VALIDATE_NON_NULL(msg);
    VALIDATE_NON_NULL(value);

    for (i = 0; i < size; i++)
    {
        bytesRequired += strlen(value[i]) + 1;
    }

    // Ensure the buffer is big enough for this
    allocateBufferMemory ((void**)&impl->mField.mBuffer,
                          &impl->mField.mBufferLen,
                          bytesRequired);

    uint8_t* target = (uint8_t*)impl->mField.mBuffer;
    for (i = 0; i < size; i++)
    {
        size_t copyLen = strlen(value[i]) + 1;
        memcpy(target, value[i], copyLen);
        target += copyLen;
    }

    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_VECTOR_STRING,
                                                  name,
                                                  fid,
                                                  (uint8_t*)impl->mField.mBuffer,
                                                  bytesRequired);
}

mama_status
omnmmsgPayload_updateVectorBool (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_bool_t   value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_bool_t, MAMA_FIELD_TYPE_VECTOR_BOOL);
}

mama_status
omnmmsgPayload_updateVectorChar (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const char          value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, char, MAMA_FIELD_TYPE_VECTOR_CHAR);
}

mama_status
omnmmsgPayload_updateVectorI8   (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i8_t     value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_i8_t, MAMA_FIELD_TYPE_VECTOR_I8);
}

mama_status
omnmmsgPayload_updateVectorU8   (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u8_t     value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_u8_t, MAMA_FIELD_TYPE_VECTOR_U8);
}

mama_status
omnmmsgPayload_updateVectorI16  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i16_t    value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_i16_t, MAMA_FIELD_TYPE_VECTOR_I16);
}

mama_status
omnmmsgPayload_updateVectorU16  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u16_t    value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_u16_t, MAMA_FIELD_TYPE_VECTOR_U16);
}

mama_status
omnmmsgPayload_updateVectorI32  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i32_t    value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_i32_t, MAMA_FIELD_TYPE_VECTOR_I32);
}

mama_status
omnmmsgPayload_updateVectorU32  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u32_t    value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_u32_t, MAMA_FIELD_TYPE_VECTOR_U32);
}

mama_status
omnmmsgPayload_updateVectorI64  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i64_t    value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_i64_t, MAMA_FIELD_TYPE_VECTOR_I64);
}

mama_status
omnmmsgPayload_updateVectorU64  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u64_t    value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_u64_t, MAMA_FIELD_TYPE_VECTOR_U64);
}

mama_status
omnmmsgPayload_updateVectorF32  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_f32_t    value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_f32_t, MAMA_FIELD_TYPE_VECTOR_F32);
}

mama_status
omnmmsgPayload_updateVectorF64  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_f64_t    value[],
                                 mama_size_t         size)
{
    UPDATE_SCALAR_VECTOR(msg, name, fid, (mama_bool_t*)value, size, mama_f64_t, MAMA_FIELD_TYPE_VECTOR_F64);
}

mama_status
omnmmsgPayload_updateVectorPrice (msgPayload          msg,
                                  const char*         name,
                                  mama_fid_t          fid,
                                  const mamaPrice     value[],
                                  mama_size_t         size)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*)msg;
    size_t i = 0;
    VALIDATE_NAME_FID(name, fid);
    VALIDATE_NON_NULL(msg);
    VALIDATE_NON_NULL(value);

    // Ensure the buffer is big enough for this
    allocateBufferMemory((void**)&impl->mField.mBuffer,
        &impl->mField.mBufferLen,
        size * sizeof(omnmPrice));

    omnmPrice* prices = (omnmPrice*)impl->mField.mBuffer;
    for (i = 0; i < size; i++)
    {
        OmnmPayloadImpl::convertMamaPriceToOmnmPrice(value[i], &prices[i]);
    }

    return ((OmnmPayloadImpl*)msg)->updateField(MAMA_FIELD_TYPE_VECTOR_PRICE,
        name,
        fid,
        (uint8_t*)impl->mField.mBuffer,
        size * sizeof(omnmPrice));
}

mama_status
omnmmsgPayload_updateVectorTime (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaDateTime  value[],
                                 mama_size_t         size)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*)msg;
    size_t i = 0;
    VALIDATE_NAME_FID(name, fid);
    VALIDATE_NON_NULL(msg);
    VALIDATE_NON_NULL(value);

    // Ensure the buffer is big enough for this
    allocateBufferMemory((void**)&impl->mField.mBuffer,
        &impl->mField.mBufferLen,
        size * sizeof(omnmDateTime));

    omnmDateTime* dateTimes = (omnmDateTime*)impl->mField.mBuffer;
    for (i = 0; i < size; i++)
    {
        OmnmPayloadImpl::convertMamaDateTimeToOmnmDateTime(value[i], &dateTimes[i]);
    }

    return ((OmnmPayloadImpl*)msg)->updateField(MAMA_FIELD_TYPE_VECTOR_TIME,
        name,
        fid,
        (uint8_t*)impl->mField.mBuffer,
        size * sizeof(omnmDateTime));
}


mama_status
omnmmsgPayload_getBool (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_bool_t*        result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_BOOL, name, fid, result);
}

mama_status
omnmmsgPayload_getChar (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        char*               result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_CHAR, name, fid, result);
}

mama_status
omnmmsgPayload_getI8   (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_i8_t*          result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_I8, name, fid, result);
}

mama_status
omnmmsgPayload_getU8   (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_u8_t*          result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_U8, name, fid, result);
}

mama_status
omnmmsgPayload_getI16  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_i16_t*         result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_I16, name, fid, result);
}

mama_status
omnmmsgPayload_getU16  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_u16_t*         result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_U16, name, fid, result);
}

mama_status
omnmmsgPayload_getI32  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_i32_t*         result)
{
    if (NULL == msg) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_I32, name, fid, result);
}

mama_status
omnmmsgPayload_getU32  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_u32_t*         result)
{
    if (NULL == msg) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_U32, name, fid, result);
}

mama_status
omnmmsgPayload_getI64  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_i64_t*         result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_I64, name, fid, result);

}

mama_status
omnmmsgPayload_getU64  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_u64_t*         result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_U64, name, fid, result);
}

mama_status
omnmmsgPayload_getF32  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_f32_t*         result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_F32, name, fid, result);
}

mama_status
omnmmsgPayload_getF64  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_f64_t*         result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsCopy(MAMA_FIELD_TYPE_F64, name, fid, result);
}

mama_status
omnmmsgPayload_getString (const msgPayload    msg,
                          const char*         name,
                          mama_fid_t          fid,
                          const char**        result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    return ((OmnmPayloadImpl *) msg)->getFieldValueAsBuffer (MAMA_FIELD_TYPE_STRING, name, fid, result);
}

mama_status
omnmmsgPayload_getOpaque (const msgPayload    msg,
                          const char*         name,
                          mama_fid_t          fid,
                          const void**        result,
                          mama_size_t*        size)
{
    omnmFieldImpl field;
    mama_status status;
    if (NULL == msg || NULL == result || NULL == size) return MAMA_STATUS_NULL_ARG;
    status = ((OmnmPayloadImpl *) msg)->getField (name, fid, field);
    if (MAMA_STATUS_OK == status)
    {
        *result = (void*) field.mData;
        *size = (mama_size_t) field.mSize;
    }
    return status;
}

mama_status
omnmmsgPayload_getDateTime (const msgPayload    msg,
                            const char*         name,
                            mama_fid_t          fid,
                            mamaDateTime        result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*) msg;
    mama_status status = impl->getField (name, fid, impl->mField);
    if (MAMA_STATUS_OK != status)
    {
        return status;
    }

    return omnmmsgFieldPayload_getDateTime ((const msgFieldPayload)&impl->mField, result);
}

mama_status
omnmmsgPayload_getPrice (const msgPayload    msg,
                         const char*         name,
                         mama_fid_t          fid,
                         mamaPrice           result)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl *) msg;
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    mama_status status = impl->getField (name, fid, impl->mField);
    if (MAMA_STATUS_OK != status)
    {
        return status;
    }

    return omnmmsgFieldPayload_getPrice((const msgFieldPayload)&impl->mField, result);
}

mama_status
omnmmsgPayload_getMsg (const msgPayload    msg,
                       const char*         name,
                       mama_fid_t          fid,
                       msgPayload*         result)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl *) msg;
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;

    impl->getField (name, fid, impl->mField);

    return omnmmsgFieldPayload_getMsg ((msgFieldPayload)&impl->mField, result);
}

mama_status
omnmmsgPayload_getVectorBool   (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_bool_t** result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_bool_t);
}

mama_status
omnmmsgPayload_getVectorChar   (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char**        result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, char);
}

mama_status
omnmmsgPayload_getVectorI8     (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i8_t**   result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_i8_t);
}

mama_status
omnmmsgPayload_getVectorU8     (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u8_t**   result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_u8_t);
}

mama_status
omnmmsgPayload_getVectorI16    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i16_t**  result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_i16_t);
}

mama_status
omnmmsgPayload_getVectorU16    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u16_t**  result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_u16_t);
}

mama_status
omnmmsgPayload_getVectorI32    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i32_t**  result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_i32_t);
}

mama_status
omnmmsgPayload_getVectorU32    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u32_t**  result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_u32_t);
}

mama_status
omnmmsgPayload_getVectorI64    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i64_t**  result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_i64_t);
}

mama_status
omnmmsgPayload_getVectorU64    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u64_t**  result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_u64_t);
}

mama_status
omnmmsgPayload_getVectorF32    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_f32_t**  result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_f32_t);
}

mama_status
omnmmsgPayload_getVectorF64    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_f64_t**  result,
                                mama_size_t*        size)
{
    GET_SCALAR_VECTOR(msg, name, fid, result, size, mama_f64_t);
}

mama_status
omnmmsgPayload_getVectorString (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char***       result,
                                mama_size_t*        size)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl *) msg;
    VALIDATE_NAME_FID(name, fid);
    VALIDATE_NON_NULL(msg);

    /* populate field with result */
    mama_status status = impl->getField (name, fid, impl->mField);

    if (MAMA_STATUS_OK != status) return status;

    return omnmmsgFieldPayload_getVectorString ((const msgFieldPayload)&impl->mField, result, size);
}

mama_status
omnmmsgPayload_getVectorDateTime (const msgPayload     msg,
                                  const char*          name,
                                  mama_fid_t           fid,
                                  const mamaDateTime** result,
                                  mama_size_t*         size)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl *)msg;
    VALIDATE_NAME_FID(name, fid);
    VALIDATE_NON_NULL(msg);

    /* populate field with result */
    mama_status status = impl->getField(name, fid, impl->mField);

    if (MAMA_STATUS_OK != status) return status;

    return omnmmsgFieldPayload_getVectorDateTime(&impl->mField, result, size);
}

mama_status
omnmmsgPayload_getVectorPrice (const msgPayload    msg,
                               const char*         name,
                               mama_fid_t          fid,
                               const mamaPrice**   result,
                               mama_size_t*        size)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl *)msg;
    VALIDATE_NAME_FID(name, fid);
    VALIDATE_NON_NULL(msg);

    /* populate field with result */
    mama_status status = impl->getField(name, fid, impl->mField);

    if (MAMA_STATUS_OK != status) return status;

    return omnmmsgFieldPayload_getVectorPrice(&impl->mField, result, size);
}

mama_status
omnmmsgPayload_getVectorMsg (const msgPayload    msg,
                             const char*         name,
                             mama_fid_t          fid,
                             const msgPayload**  result,
                             mama_size_t*        size)
{
    OmnmPayloadImpl* impl = (OmnmPayloadImpl *) msg;
    VALIDATE_NAME_FID(name, fid);
    VALIDATE_NON_NULL(msg);

    /* populate field with result */
    mama_status status = impl->getField (name, fid, impl->mField);

    if (MAMA_STATUS_OK != status) return status;

    return omnmmsgFieldPayload_getVectorMsg (&impl->mField, result, size);
}

mama_status
omnmmsgPayload_getField (const msgPayload    msg,
                         const char*         name,
                         mama_fid_t          fid,
                         msgFieldPayload*    result)
{
    if (NULL == msg || NULL == result) return MAMA_STATUS_NULL_ARG;
    OmnmPayloadImpl* impl = ((OmnmPayloadImpl*) msg);

    mama_status status = impl->getField (name, fid, impl->mField);

    *result = (msgFieldPayload) &impl->mField;
    return status;
}


mama_status
omnmmsgPayloadImpl_setExtenderClosure (msgPayload msg, void* closure)
{
    if (nullptr == msg || nullptr == closure) return MAMA_STATUS_NULL_ARG;
    ((OmnmPayloadImpl*) msg)->mExtenderClosure = closure;
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayloadImpl_getExtenderClosure (msgPayload msg, const void** closure)
{
    if (nullptr == msg || nullptr == closure) return MAMA_STATUS_NULL_ARG;
    *closure = ((OmnmPayloadImpl*) msg)->mExtenderClosure;
    return MAMA_STATUS_OK;
}