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

#include <mama/mama.h>
#include <mama/price.h>

#include "payloadbridge.h"
#include "msgfieldimpl.h"

#include "omnmmsgpayloadfunctions.h"
#include <wombat/strutils.h>
#include <wombat/memnode.h>
#include <stddef.h>
#include <priceimpl.h>
#include "Payload.h"
#include "Iterator.h"

/*=========================================================================
  =                              Macros                                   =
  =========================================================================*/
#define        FIELD_TYPE_WIDTH        1
#define        FID_WIDTH               2
#define        LENGTH_WIDTH            4
#define        DEFAULT_PAYLOAD_SIZE    200

#define UPDATE_SCALAR_FIELD(SUFFIX,MAMATYPE)                                   \
do                                                                             \
{                                                                              \
    MAMATYPE result;                                                           \
    omnmmsgFieldPayload_get##SUFFIX (field, &result);                          \
    omnmmsgPayload_update##SUFFIX   (dest, name, fid, result);                 \
} while (0)                                                                    \

#define UPDATE_VECTOR_FIELD(SUFFIX,MAMATYPE)                                   \
do                                                                             \
{                                                                              \
    const MAMATYPE * result = NULL;                                            \
    mama_size_t size = 0;                                                      \
    omnmmsgFieldPayload_getVector##SUFFIX (field, &result, &size);             \
    omnmmsgPayload_updateVector##SUFFIX   (dest, name, fid, result, size);     \
} while (0)                                                                    \

/*=========================================================================
  =                  Private implementation prototypes                    =
  =========================================================================*/


OmnmPayloadImpl::OmnmPayloadImpl() : mPayloadBuffer(NULL),
                                     mPayloadBufferSize(0),
                                     mPayloadBufferTail(0)
{
    mPayloadBufferSize = DEFAULT_PAYLOAD_SIZE;
    mPayloadBuffer     = (uint8_t*) calloc (mPayloadBufferSize, 1);

    // First byte always contains payload type - populate and move past
    *((uint8_t*)mPayloadBuffer) = MAMA_PAYLOAD_OMNM;
    mPayloadBufferTail++;
}

OmnmPayloadImpl::~OmnmPayloadImpl()
{
    if (NULL != mPayloadBuffer)
    {
        free (mPayloadBuffer);
    }
}

mama_status
OmnmPayloadImpl::clear()
{
    // Make sure tail is set to after the 'type' byte
    mPayloadBufferTail = 1;

    // NULL initialize the buffer after the first byte
    memset ((void*)(mPayloadBuffer + 1), 0, mPayloadBufferSize - 1);

    return MAMA_STATUS_OK;
}

mama_status
OmnmPayloadImpl::findFieldInBuffer (const char* name, mama_fid_t fid, omnmFieldImpl& field)
{
    // Initialize the iterator struct on the stack for speed
    omnmIterImpl   iter;
    omnmFieldImpl* fieldCandidate;

    // NULL initialize any provided field
    memset (&field, 0, sizeof(field));

    // This is really just for type casting so we can easily use bridge
    // iterator methods directly
    msgPayloadIter iterOpaque = (msgPayloadIter)&iter;

    // Initialize the iterator for this message
    omnmmsgPayloadIterImpl_init (&iter, this);

    // Iterate over all fields until
    while (NULL != (fieldCandidate = (omnmFieldImpl*)omnmmsgPayloadIter_next (iterOpaque, NULL, this)))
    {
        // Fid match - found the field
        if (fid == fieldCandidate->mFid ||
                (name != NULL && fieldCandidate->mName != NULL &&
                        0 == strcmp (name, fieldCandidate->mName)))
        {
            // Copy the field's data to the provided onmnFieldImpl
            memcpy (&field, fieldCandidate, sizeof(omnmFieldImpl));
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
    // We will insert at the tail
    uint8_t* insertPoint = mPayloadBuffer + mPayloadBufferTail;

    // New tail will be comprised of type, fid, field
    size_t newTail = mPayloadBufferTail + FIELD_TYPE_WIDTH + FID_WIDTH +
            strlenEx(name) + 1 + bufferLen;

    // Ensure the buffer is big enough for this
    allocateBufferMemory ((void**)&mPayloadBuffer,
                          &mPayloadBufferSize,
                          newTail);

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

    // Copy across the data itself
    memcpy ((void*)insertPoint, (void*)buffer, bufferLen);

    // Update the tail position
    mPayloadBufferTail = newTail;

    // If name and fid is null, add 'bare'
    return MAMA_STATUS_OK;
}

mama_status
OmnmPayloadImpl::updateField (mamaFieldType type, const char* name,
        mama_fid_t fid, uint8_t* buffer, size_t bufferLen)
{
    omnmFieldImpl field;
    mama_status   status = findFieldInBuffer (name, fid, field);

    // Add field if it can't be found
    if (MAMA_STATUS_OK != status)
    {
        return addField (type, name, fid, buffer, bufferLen);
    }

    // If the field exists, try updating it
    return updateField (field, buffer, bufferLen);
}

mama_status
OmnmPayloadImpl::updateField (omnmFieldImpl& field, uint8_t* buffer,
        size_t bufferLen)
{
    // If buffer needs to expand or shrink
    if (bufferLen != field.mSize)
    {
        int32_t delta = bufferLen - field.mSize;

        // This will be the new offset of next field after the move
        size_t nextByteOffset = ((uint8_t*)field.mData - mPayloadBuffer) +
                field.mSize;

        // Increase the buffer *if necessary*
        if (delta > 0)
        {
            // Cache pointer indices from main buffer
            int dataOffset = (uint8_t*)field.mData - mPayloadBuffer;
            int nameOffset = (uint8_t*)field.mName - mPayloadBuffer;

            // Increase buffer memory
            allocateBufferMemory ((void**)&mPayloadBuffer,
                                  &mPayloadBufferSize,
                                  mPayloadBufferSize + delta);

            // Assume buffers have moved and re-apply offsets
            field.mData = mPayloadBuffer + dataOffset;
            field.mName = (const char*)mPayloadBuffer + nameOffset;
        }

        // If the field has changed in size, we need to move memory
        if (delta != 0)
        {
            // This will point to the first field after insertion
            uint8_t* origin = mPayloadBuffer + nextByteOffset;
            // This will point to the number of remaining bytes after this field
            uint32_t size   = mPayloadBufferTail - nextByteOffset;
            // Finally move the memory across
            memmove ((void*)(origin + delta), origin, size);
        }
        mPayloadBufferTail += delta;
    }
    memcpy ((void*)field.mData, (void*)buffer, bufferLen);
    return MAMA_STATUS_OK;
}


/*=========================================================================
  =                   Public interface functions                          =
  =========================================================================*/

mama_status
omnmmsgPayload_createImpl (mamaPayloadBridge* result, char* identifier)
{
    mamaPayloadBridgeImpl* impl = NULL;

    impl = (mamaPayloadBridgeImpl*) calloc (1, sizeof (mamaPayloadBridgeImpl));

    if (NULL == impl)
    {
        mama_log (MAMA_LOG_LEVEL_SEVERE, "qpidmsgPayload_createImpl(): "
                  "Could not allocate memory for payload impl.");
        return MAMA_STATUS_NOMEM;
    }

    /* Use closure to store global data for payload bridge if required */
    impl->mClosure = NULL;

    /* Initialize the virtual function table (see payloadbridge.h) */
    INITIALIZE_PAYLOAD_BRIDGE (impl, omnmmsg);

    *result     = (mamaPayloadBridge)impl;
    *identifier = (char)MAMA_PAYLOAD_OMNM;

    return MAMA_STATUS_OK;
}

mamaPayloadType
omnmmsgPayload_getType (void)
{
    return MAMA_PAYLOAD_OMNM;
}

mama_status
omnmmsgPayload_create (msgPayload* msg)
{
    OmnmPayloadImpl* impl = new OmnmPayloadImpl();

    *msg = (msgPayload) impl;

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_destroy (msgPayload msg)
{
    delete (OmnmPayloadImpl*) msg;
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
    return ((OmnmPayloadImpl*) msg)->clear();
}

mama_status
omnmmsgPayload_setParent (msgPayload    msg,
                          const mamaMsg parent)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getByteSize (msgPayload    msg,
                            mama_size_t*  size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getNumFields (const msgPayload    msg,
                             mama_size_t*        numFields)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
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
    char                part[1024];

    // This is really just for type casting so we can easily use bridge
    // iterator methods directly
    msgPayloadIter iterOpaque = (msgPayloadIter)&iter;

    // Initialize the iterator for this message
    omnmmsgPayloadIterImpl_init (&iter, impl);

    // Iterate over all fields
    while (NULL != (fieldPayload = omnmmsgPayloadIter_next(iterOpaque, NULL, msg)))
    {
        // TODO: actually build the string
        mama_fid_t      fid = 0;
        const char*     fname = NULL;
        omnmmsgFieldPayload_getAsString(fieldPayload, impl, part, sizeof(part));
        omnmmsgFieldPayload_getFid(fieldPayload, NULL, NULL, &fid);
        omnmmsgFieldPayload_getName (fieldPayload, NULL, NULL, &fname);
    }
    return NULL;
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
    mamaMsgFieldImpl*   mamaField       = (mamaMsgFieldImpl*) field;
    msgFieldPayload     fieldPayload    = NULL;

    // This is really just for type casting so we can easily use bridge
    // iterator methods directly
    msgPayloadIter iterOpaque = (msgPayloadIter)&iter;

    // Initialize the iterator for this message
    omnmmsgPayloadIterImpl_init (&iter, impl);

    while (NULL != (fieldPayload = omnmmsgPayloadIter_next(iterOpaque, NULL, msg)))
    {
        mamaMsgFieldImpl_setPayload (mamaField, fieldPayload);
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

    /* Wipe current buffer */
    memset (impl->mPayloadBuffer, 0, impl->mPayloadBufferSize);

    /* Ensure buffer is big enough to hold */
    if (0 != allocateBufferMemory ((void**) &impl->mPayloadBuffer,
                                   &impl->mPayloadBufferSize,
                                   bufferLength))
    {
        return MAMA_STATUS_NOMEM;
    }
    memcpy (impl->mPayloadBuffer, (void*)buffer, bufferLength);

    impl->mPayloadBufferTail = bufferLength;

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayload_getByteBuffer (const msgPayload  msg,
                              const void**      buffer,
                              mama_size_t*      bufferLength)
{

    return MAMA_STATUS_NOT_IMPLEMENTED;
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
    return omnmmsgPayload_unSerialize (msg, (const void**)buffer, bufferLength);
}

mama_status
omnmmsgPayload_createFromByteBuffer (msgPayload*         msg,
                                     mamaPayloadBridge   bridge,
                                     const void*         buffer,
                                     mama_size_t         bufferLength)
{
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
            UPDATE_SCALAR_FIELD (Bool, mama_bool_t);
            break;
        }
        case MAMA_FIELD_TYPE_CHAR:
        {
            UPDATE_SCALAR_FIELD (Char, char);
            break;
        }
        case MAMA_FIELD_TYPE_U8:
        {
            UPDATE_SCALAR_FIELD (U8, mama_u8_t);
            break;
        }
        case MAMA_FIELD_TYPE_I8:
        {
            UPDATE_SCALAR_FIELD (I8, mama_i8_t);
            break;
        }
        case MAMA_FIELD_TYPE_U16:
        {
            UPDATE_SCALAR_FIELD (U16, mama_u16_t);
            break;
        }
        case MAMA_FIELD_TYPE_I16:
        {
            UPDATE_SCALAR_FIELD (I16, mama_i16_t);
            break;
        }
        case MAMA_FIELD_TYPE_U32:
        {
            UPDATE_SCALAR_FIELD (U32, mama_u32_t);
            break;
        }
        case MAMA_FIELD_TYPE_I32:
        {
            UPDATE_SCALAR_FIELD (I32, mama_i32_t);
            break;
        }
        case MAMA_FIELD_TYPE_U64:
        {
            UPDATE_SCALAR_FIELD (U64, mama_u64_t);
            break;
        }
        case MAMA_FIELD_TYPE_I64:
        {
            UPDATE_SCALAR_FIELD (I64, mama_i64_t);
            break;
        }
        case MAMA_FIELD_TYPE_F32:
        {
            UPDATE_SCALAR_FIELD (F32, mama_f32_t);
            break;
        }
        case MAMA_FIELD_TYPE_F64:
        {
            UPDATE_SCALAR_FIELD (F64, mama_f64_t);
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
            omnmmsgPayload_updateSubMsg (dest, name, fid, result);
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
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getFieldAsString (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 char*               buffer,
                                 mama_size_t         len)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addBool (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_bool_t value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_BOOL, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addChar (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        char        value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_CHAR, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addI8   (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_i8_t   value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_I8, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addU8   (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_u8_t   value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_U8, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addI16  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_i16_t  value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_I16, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addU16  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_u16_t  value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_U16, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addI32  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_i32_t  value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_I32, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addU32  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_u32_t  value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_U32, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addI64  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_i64_t  value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_I64, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addU64  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_u64_t  value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_U64, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addF32  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_f32_t  value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_F32, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addF64  (msgPayload  msg,
                        const char* name,
                        mama_fid_t  fid,
                        mama_f64_t  value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_F64, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_addString (msgPayload  msg,
                          const char* name,
                          mama_fid_t  fid,
                          const char* str)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_STRING, name, fid, (uint8_t*)str, strlenEx(str) + 1);
}

mama_status
omnmmsgPayload_addOpaque (msgPayload  msg,
                          const char* name,
                          mama_fid_t  fid,
                          const void* opaque,
                          mama_size_t size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addDateTime (msgPayload          msg,
                            const char*         name,
                            mama_fid_t          fid,
                            const mamaDateTime  value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_TIME, name, fid, (uint8_t*)value, sizeof(*value));
}

mama_status
omnmmsgPayload_addPrice (msgPayload      msg,
                         const char*     name,
                         mama_fid_t      fid,
                         const mamaPrice value)
{
    return ((OmnmPayloadImpl*) msg)->addField (MAMA_FIELD_TYPE_PRICE, name, fid, (uint8_t*)value, sizeof(mama_price_t));
}

mama_status
omnmmsgPayload_addMsg (msgPayload  msg,
                       const char* name,
                       mama_fid_t  fid,
                       msgPayload  value)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorBool (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_bool_t   value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorChar (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const char          value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorI8   (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_i8_t     value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorU8   (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_u8_t     value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorI16  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_i16_t    value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorU16  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_u16_t    value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorI32  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_i32_t    value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorU32  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_u32_t    value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorI64  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_i64_t    value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorU64  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_u64_t    value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorF32  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_f32_t    value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorF64  (msgPayload          msg,
                              const char*         name,
                              mama_fid_t          fid,
                              const mama_f64_t    value[],
                              mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorString (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char*         value[],
                                mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_addVectorMsg (msgPayload          msg,
                             const char*         name,
                             mama_fid_t          fid,
                             const mamaMsg       value[],
                             mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

/*
 * Postponing implementation until this type of vectors has a standard protocol
 * or is removed from the implementation
 */
mama_status
omnmmsgPayload_addVectorDateTime (msgPayload          msg,
                                  const char*         name,
                                  mama_fid_t          fid,
                                  const mamaDateTime  value[],
                                  mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

/*
 * Postponing implementation until this type of vectors has a standard protocol
 * or is removed from the implementation
 */
mama_status
omnmmsgPayload_addVectorPrice (msgPayload      msg,
                               const char*     name,
                               mama_fid_t      fid,
                               const mamaPrice value[],
                               mama_size_t     size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateBool   (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_bool_t  value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_BOOL, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateChar   (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             char         value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_CHAR, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateI8     (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_i8_t    value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_I8, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateU8     (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_u8_t    value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_U8, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateI16    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_i16_t   value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_I16, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateU16    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_u16_t   value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_U16, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateI32    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_i32_t   value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_I32, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateU32    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_u32_t   value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_U32, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateI64    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_i64_t   value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_I64, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateU64    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_u64_t   value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_U64, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateF32    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_f32_t   value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_F32, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateF64    (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             mama_f64_t   value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_F64, name, fid, (uint8_t*)&value, sizeof(value));
}

mama_status
omnmmsgPayload_updateString (msgPayload   msg,
                             const char*  name,
                             mama_fid_t   fid,
                             const char*  str)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_STRING, name, fid, (uint8_t*)str, strlenEx(str) + 1);
}

mama_status
omnmmsgPayload_updateOpaque (msgPayload          msg,
                             const char*         name,
                             mama_fid_t          fid,
                             const void*         opaque,
                             mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateDateTime (msgPayload          msg,
                               const char*         name,
                               mama_fid_t          fid,
                               const mamaDateTime  value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_TIME, name, fid, (uint8_t*)value, sizeof(*value));
}

mama_status
omnmmsgPayload_updatePrice (msgPayload          msg,
                            const char*         name,
                            mama_fid_t          fid,
                            const mamaPrice     value)
{
    return ((OmnmPayloadImpl*) msg)->updateField (MAMA_FIELD_TYPE_STRING, name, fid, (uint8_t*)value, sizeof(mama_price_t));
}

mama_status
omnmmsgPayload_updateSubMsg (msgPayload          msg,
                             const char*         name,
                             mama_fid_t          fid,
                             const msgPayload    subMsg)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorMsg (msgPayload          msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mamaMsg       value[],
                                mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorString (msgPayload   msg,
                                   const char*  name,
                                   mama_fid_t   fid,
                                   const char*  value[],
                                   mama_size_t  size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorBool (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_bool_t   value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorChar (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const char          value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorI8   (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i8_t     value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorU8   (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u8_t     value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorI16  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i16_t    value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorU16  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u16_t    value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorI32  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i32_t    value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorU32  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u32_t    value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorI64  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i64_t    value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorU64  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u64_t    value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorF32  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_f32_t    value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorF64  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_f64_t    value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorPrice (msgPayload          msg,
                                  const char*         name,
                                  mama_fid_t          fid,
                                  const mamaPrice*    value[],
                                  mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_updateVectorTime (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaDateTime  value[],
                                 mama_size_t         size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}


mama_status
omnmmsgPayload_getBool (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_bool_t*        result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_BOOL, name, fid, result);
}

mama_status
omnmmsgPayload_getChar (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        char*               result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_CHAR, name, fid, result);
}

mama_status
omnmmsgPayload_getI8   (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_i8_t*          result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_I8, name, fid, result);
}

mama_status
omnmmsgPayload_getU8   (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_u8_t*          result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_U8, name, fid, result);
}

mama_status
omnmmsgPayload_getI16  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_i16_t*         result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_I16, name, fid, result);
}

mama_status
omnmmsgPayload_getU16  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_u16_t*         result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_U16, name, fid, result);
}

mama_status
omnmmsgPayload_getI32  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_i32_t*         result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_I32, name, fid, result);
}

mama_status
omnmmsgPayload_getU32  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_u32_t*         result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_U32, name, fid, result);
}

mama_status
omnmmsgPayload_getI64  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_i64_t*         result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_I64, name, fid, result);

}

mama_status
omnmmsgPayload_getU64  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_u64_t*         result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_U64, name, fid, result);
}

mama_status
omnmmsgPayload_getF32  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_f32_t*         result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_F32, name, fid, result);
}

mama_status
omnmmsgPayload_getF64  (const msgPayload    msg,
                        const char*         name,
                        mama_fid_t          fid,
                        mama_f64_t*         result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_F64, name, fid, result);
}

mama_status
omnmmsgPayload_getString (const msgPayload    msg,
                          const char*         name,
                          mama_fid_t          fid,
                          const char**        result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_STRING, name, fid, result);
}

mama_status
omnmmsgPayload_getOpaque (const msgPayload    msg,
                          const char*         name,
                          mama_fid_t          fid,
                          const void**        result,
                          mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getDateTime (const msgPayload    msg,
                            const char*         name,
                            mama_fid_t          fid,
                            mamaDateTime        result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_TIME, name, fid, result);
}

mama_status
omnmmsgPayload_getPrice (const msgPayload    msg,
                         const char*         name,
                         mama_fid_t          fid,
                         mamaPrice           result)
{
    return ((OmnmPayloadImpl*) msg)->getField(MAMA_FIELD_TYPE_PRICE, name, fid, (mama_price_t*)result);
}

mama_status
omnmmsgPayload_getMsg (const msgPayload    msg,
                       const char*         name,
                       mama_fid_t          fid,
                       msgPayload*         result)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorBool   (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_bool_t** result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorChar   (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char**        result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorI8     (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i8_t**   result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorU8     (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u8_t**   result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorI16    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i16_t**  result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorU16    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u16_t**  result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorI32    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i32_t**  result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorU32    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u32_t**  result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorI64    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_i64_t**  result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorU64    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_u64_t**  result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorF32    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_f32_t**  result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorF64    (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const mama_f64_t**  result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorString (const msgPayload    msg,
                                const char*         name,
                                mama_fid_t          fid,
                                const char***       result,
                                mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorDateTime (const msgPayload    msg,
                                  const char*         name,
                                  mama_fid_t          fid,
                                  const mamaDateTime* result,
                                  mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorPrice (const msgPayload    msg,
                               const char*         name,
                               mama_fid_t          fid,
                               const mamaPrice*    result,
                               mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getVectorMsg (const msgPayload    msg,
                             const char*         name,
                             mama_fid_t          fid,
                             const msgPayload**  result,
                             mama_size_t*        size)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgPayload_getField (const msgPayload    msg,
                         const char*         name,
                         mama_fid_t          fid,
                         msgFieldPayload*    result)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}


