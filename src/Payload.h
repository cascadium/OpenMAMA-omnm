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

#ifndef MAMA_BRIDGE_OMNM_MSG_PAYLOAD_H__
#define MAMA_BRIDGE_OMNM_MSG_PAYLOAD_H__

#include <mama/mama.h>
#include <mama/price.h>
#include <wombat/strutils.h>

class OmnmPayloadImpl;

#define VALIDATE_NAME_FID(NAME, FID)                                           \
do                                                                             \
{                                                                              \
    if (NULL == NAME && 0 == FID)                                              \
        return MAMA_STATUS_NULL_ARG;                                           \
    else if (0 == strlenEx(NAME) && 0 == FID)                                  \
        return MAMA_STATUS_INVALID_ARG;                                        \
} while (0)

typedef struct omnmFieldImpl
{
    mamaFieldType       mFieldType;
    mama_fid_t          mFid;
    const char*         mName;
    uint32_t            mSize;
    void*               mData;
    OmnmPayloadImpl*    mParent;
    /* Complex data type elements below */
    msgPayload          mSubPayload;
    const char**        mVectorString;
    mama_size_t         mVectorStringLen;
    msgPayload*         mVectorPayload;
    mama_size_t         mVectorPayloadLen;
} omnmFieldImpl;

class OmnmPayloadImpl {
public:
    OmnmPayloadImpl();
    ~OmnmPayloadImpl();

    // Templatized function for casting buffers to data types by lookup
    // Receives type, fid and name as an argument and returns pointer to data
    // Contains typs as additional thing that may possibly be verified. Might
    // end up removing this function
    template<typename T>
    mama_status
    getFieldValueAsCopy(mamaFieldType type, const char *name, mama_fid_t fid, T *s)
    {
        omnmFieldImpl field;
        VALIDATE_NAME_FID(name, fid);
        mama_status status = findFieldInBuffer (name, fid, field);
        if (MAMA_STATUS_OK != status)
        {
            return MAMA_STATUS_NOT_FOUND;
        }
        // Call known based templatized function
        return getFieldValueAsCopy (field, s);
    }

    // Templatized function for casting buffers to data types by known field
    // Receives field with fid and name as an argument and returns pointer to data
    template<typename T>
    mama_status
    getFieldValueAsCopy(struct omnmFieldImpl &field, T *s)
    {
        VALIDATE_NAME_FID(field.mName, field.mFid);
        memset (s, 0, sizeof(T));

        // If size on wire doesn't match requested size, return error
        if (field.mSize > sizeof(T))
        {
            return MAMA_STATUS_WRONG_FIELD_TYPE;
        }
        memcpy (s, field.mData, field.mSize);

        return MAMA_STATUS_OK;
    }

    // Templatized function for casting buffers to data types by lookup
    template<typename T>
    mama_status
    getFieldValueAsBuffer(mamaFieldType type, const char *name, mama_fid_t fid, T **s)
    {
        omnmFieldImpl field;
        VALIDATE_NAME_FID(name, fid);
        mama_status status = findFieldInBuffer (name, fid, field);
        if (MAMA_STATUS_OK != status)
        {
            return MAMA_STATUS_NOT_FOUND;
        }

        // Call known based templatized function
        return getFieldValueAsBuffer(field, s);
    }

    // Templatized function for casting buffers to data types by known field
    template<typename T>
    mama_status
    getFieldValueAsBuffer(omnmFieldImpl &field, T **s)
    {
        VALIDATE_NAME_FID(field.mName, field.mFid);
        // If data type is a pointer, simply point it to the data buffer
        *s = ((T*) field.mData);

        return MAMA_STATUS_OK;
    }

    mama_status
    getField(const char* name, mama_fid_t fid, omnmFieldImpl& field)
    {
        VALIDATE_NAME_FID(name, fid);
        mama_status status = this->findFieldInBuffer (name, fid, field);
        if (MAMA_STATUS_OK != status)
        {
            return MAMA_STATUS_NOT_FOUND;
        }

        return MAMA_STATUS_OK;
    }

    // Add payload field according to the type and values provided
    mama_status
    addField    (mamaFieldType  type,
                 const char*    name,
                 mama_fid_t     fid,
                 uint8_t*       buffer,
                 size_t         bufferLen);

    // Update payload field according to the type and values provided
    mama_status
    updateField (mamaFieldType   type,
                 const char*     name,
                 mama_fid_t      fid,
                 uint8_t*        buffer,
                 size_t          bufferLen);

    // Update payload field according to the field and values provided
    mama_status
    updateField (mamaFieldType          type,
                 struct omnmFieldImpl&  field,
                 uint8_t*               buffer,
                 size_t                 bufferLen);

    // Update payload field according to the field and values provided
    static bool
    isFieldTypeSized (mamaFieldType type);

    static bool
    isFieldTypeFixedWidth (mamaFieldType type);

    static bool
    areFieldTypesCastable (mamaFieldType from, mamaFieldType to);

    // Clear the payload
    mama_status clear ();

    // Underlying buffer to store the payload
    uint8_t*    mPayloadBuffer;

    // This really reflects the capacity of mPayloadBuffer
    size_t      mPayloadBufferSize;

    // Tail always points to the end of the 'useful' part of the buffer, where
    // the free space in the remaining buffer lives
    size_t      mPayloadBufferTail;

    omnmFieldImpl mField;
    mamaMsg       mParent;
private:
    // Find the field inside the buffer and populate provided field with its
    // location
    mama_status findFieldInBuffer (const char* name, mama_fid_t fid, struct omnmFieldImpl& field);
};

#endif /* MAMA_BRIDGE_OMNM_MSG_PAYLOAD_H__ */
