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
#include <stdint.h>

#include <mama/mama.h>

#include "omnmmsgpayloadfunctions.h"
#include "Payload.h"
#include "Iterator.h"

/*=========================================================================
  =                Typedefs, structs, enums and globals                   =
  =========================================================================*/


/*=========================================================================
  =                   Public implementation functions                     =
  =========================================================================*/

mama_status
omnmmsgPayloadIter_create (msgPayloadIter* iter,
                           msgPayload      msg)
{
    omnmIterImpl*        impl       = NULL;
    OmnmPayloadImpl*     msgImpl    = (OmnmPayloadImpl*) msg;

    if (NULL == msg || NULL == iter)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    impl = (omnmIterImpl*) calloc (1, sizeof (omnmIterImpl));

    impl->mField.mParent  = msgImpl;
    impl->mMsg            = msgImpl;

    /* Start iterating just after the message type byte */
    omnmmsgPayloadIter_begin (impl, NULL, msgImpl);

    *iter = impl;

    return MAMA_STATUS_OK;
}

msgFieldPayload
omnmmsgPayloadIter_next (msgPayloadIter          iter,
                         msgFieldPayload         field,
                         msgPayload              msg)
{
    omnmIterImpl*   impl    = (omnmIterImpl*) iter;

    if (NULL == iter || NULL == msg)
        return NULL;

    // If the current buffer iterator position is at or past the end
    if (! omnmmsgPayloadIter_hasNext(iter, msg))
    {
        return NULL;
    }

    // Initialize the field member
    impl->mField.mFieldType = (mamaFieldType)(*impl->mBufferPosition);
    impl->mField.mSize      = 0;
    impl->mField.mFid       = 0;
    impl->mField.mIndex     = impl->mIndex;

    // Move past the field type
    impl->mBufferPosition++;

    // Set field fid and advance buffer position
    impl->mField.mFid = *((mama_fid_t*) impl->mBufferPosition);
    impl->mBufferPosition += sizeof(uint16_t);

    // If field name is an empty string
    if (*impl->mBufferPosition != '\0')
    {
        impl->mField.mName = (const char*) impl->mBufferPosition;
        impl->mBufferPosition += impl->mMsg->mFieldHints[impl->mField.mIndex].nameLen;
    }
    else
    {
        impl->mField.mName = NULL;
        impl->mBufferPosition++;
    }

    // Populate mData with pointer to data and size with byte size
    switch (impl->mField.mFieldType)
    {
    case MAMA_FIELD_TYPE_BOOL:
    case MAMA_FIELD_TYPE_CHAR:
    case MAMA_FIELD_TYPE_I8:
    case MAMA_FIELD_TYPE_U8:
        impl->mField.mData = (void*) impl->mBufferPosition;
        impl->mField.mSize = sizeof(mama_u8_t);
        break;
    case MAMA_FIELD_TYPE_I16:
    case MAMA_FIELD_TYPE_U16:
        impl->mField.mData = (void*) impl->mBufferPosition;
        impl->mField.mSize = sizeof(mama_u16_t);
        break;
    case MAMA_FIELD_TYPE_I32:
    case MAMA_FIELD_TYPE_U32:
    case MAMA_FIELD_TYPE_F32:
    case MAMA_FIELD_TYPE_QUANTITY:
        impl->mField.mData = (void*) impl->mBufferPosition;
        impl->mField.mSize = sizeof(mama_u32_t);
        break;
    case MAMA_FIELD_TYPE_I64:
    case MAMA_FIELD_TYPE_U64:
    case MAMA_FIELD_TYPE_F64:
        impl->mField.mData = (void*) impl->mBufferPosition;
        impl->mField.mSize = sizeof(mama_u64_t);
        break;
    case MAMA_FIELD_TYPE_PRICE:
        impl->mField.mData = (void*) impl->mBufferPosition;
        impl->mField.mSize = sizeof(omnmPrice);
        break;
    case MAMA_FIELD_TYPE_TIME:
        impl->mField.mData = (void*)impl->mBufferPosition;
        impl->mField.mSize = sizeof(omnmDateTime);
        break;
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
        /* All these field types start with a U32 detailing the message size in bytes */
        impl->mField.mSize = *((mama_u32_t*)impl->mBufferPosition);

        /* 32 bit field size is variable - skip over its position */
        impl->mBufferPosition += sizeof(mama_u32_t);
        /* Note the data starts *after* the size field */
        impl->mField.mData = (void*)impl->mBufferPosition;
        break;
    case MAMA_FIELD_TYPE_UNKNOWN:
        break;
    }

    impl->mBufferPosition += impl->mField.mSize;
    impl->mIndex++;

    return &impl->mField;
}

mama_bool_t
omnmmsgPayloadIter_hasNext (msgPayloadIter          iter,
                            msgPayload              msg)
{
    omnmIterImpl*   impl    = (omnmIterImpl*) iter;
    if (NULL == iter || NULL == msg) return 0;

    // If the current buffer iterator position is at or past the end
    if (impl->mMsg->mPayloadBuffer + impl->mMsg->mPayloadBufferTail <= impl->mBufferPosition)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

msgFieldPayload
omnmmsgPayloadIter_begin (msgPayloadIter          iter,
                          msgFieldPayload         field,
                          msgPayload              msg)
{
    omnmIterImpl*   impl       = (omnmIterImpl*) iter;
    msgFieldPayload firstField = NULL;

    if (NULL == iter || NULL == msg) return NULL;

    impl->mMsg         = (OmnmPayloadImpl*)msg;
    impl->mIndex       = 0;

    /* Start iterating just after the message type byte */
    impl->mBufferPosition = impl->mMsg->mPayloadBuffer + impl->mMsg->getHeaderSize();

    firstField = omnmmsgPayloadIter_next (iter, field, msg);

    // Reset iterating back to just after the header
    impl->mIndex = 0;
    impl->mBufferPosition = impl->mMsg->mPayloadBuffer + impl->mMsg->getHeaderSize();

    return firstField;
}

/*
 * Postponing implementation until this method is included or removed from the
 * implementation
 */
msgFieldPayload
omnmmsgPayloadIter_end (msgPayloadIter          iter,
                        msgPayload              msg)
{
    return NULL;
}

mama_status
omnmmsgPayloadIter_associate (msgPayloadIter          iter,
                              msgPayload              msg)
{
    if (NULL == iter || NULL == msg) return MAMA_STATUS_NULL_ARG;
    // Reset iterator position and couple
    omnmmsgPayloadIter_begin (iter, NULL, msg);
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayloadIter_destroy (msgPayloadIter iter)
{
    if (NULL == iter) return MAMA_STATUS_NULL_ARG;
    free (iter);
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgPayloadIterImpl_init (omnmIterImpl*      iter,
                             OmnmPayloadImpl*   msg)
{
    if (NULL == msg || NULL == iter)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    // Ensure buffer provided is NULL initialized
    memset ((void*)iter, 0, sizeof(omnmIterImpl));

    // Initialize members
    iter->mField.mParent  = msg;
    iter->mMsg            = msg;

    // Reset iterator position
    omnmmsgPayloadIter_begin (iter, NULL, msg);

    return MAMA_STATUS_OK;
}

