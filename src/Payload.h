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
#include <mama/integration/types.h>

class OmnmPayloadImpl;

#define VALIDATE_MAMA_STATUS_OK(STATUS)                                        \
do                                                                             \
{                                                                              \
    if (MAMA_STATUS_OK != STATUS)                                              \
        return STATUS;                                                         \
} while (0)

#define VALIDATE_NAME_FID(NAME, FID)                                           \
do                                                                             \
{                                                                              \
    if (NULL == NAME && 0 == FID)                                              \
        return MAMA_STATUS_NULL_ARG;                                           \
    else if (0 == strlenEx(NAME) && 0 == FID)                                  \
        return MAMA_STATUS_INVALID_ARG;                                        \
} while (0)

#define VALIDATE_NON_NULL(VALUE)                                               \
do                                                                             \
{                                                                              \
    if (NULL == VALUE)                                                         \
        return MAMA_STATUS_NULL_ARG;                                           \
} while (0)

typedef struct omnmFieldImpl
{
    mamaFieldType       mFieldType;
    mama_fid_t          mFid;
    const char*         mName;
    size_t              mSize;
    void*               mData; /* never alloc'ed memory - always reference */
    OmnmPayloadImpl*    mParent;
    /* Complex data type elements below */
    void*               mBuffer; /* Reusable buffer for temporary data */
    mama_size_t         mBufferLen; /* Reusable buffer for temporary data */
    msgPayload          mSubPayload;
    const char**        mVectorString;
    mama_size_t         mVectorStringLen;
    msgPayload*         mVectorPayload;
    mama_size_t         mVectorPayloadLen;
    mamaDateTime*       mVectorDateTime;
    mama_size_t         mVectorDateTimeLen;
    mamaPrice*          mVectorPrice;
    mama_size_t         mVectorPriceLen;
} omnmFieldImpl;

typedef struct omnmDateTime
{
    mama_u8_t  mHints;             /* Contains more information on how to parse */
    mama_i64_t mSeconds;           /* -ve means time before epoch */
    mama_u32_t mNanoseconds;
    mama_u8_t  mPrecision;
    char       mTimezoneName[14];  /* https://en.wikipedia.org/wiki/Tz_database */
} omnmDateTime;

typedef struct omnmPrice
{
    mama_u8_t  mHints;             /* Contains more information on how to parse*/
    mama_f64_t mValue;
    char       mCurrency[3];       /* ISO 4217 3-character currency code */
} omnmPrice;

/*
 * byte[0]  = Payload Type
 * byte[1]  = Payload Version
 * byte[2]  = Remaining Header Bytes
 * byte[3+] = Header struct
 */
typedef struct omnmHeaderV1
{
    mama_u8_t  mType;
    mama_u8_t  mWireFormatVersion;
    mama_u8_t  mRemainingHeaderSize; /* Always 0 in this version */
} omnmHeaderV1;

// The header type is defined as a V1 header
typedef omnmHeaderV1 omnmHeader;

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
        VALIDATE_NAME_FID(name, fid);
        VALIDATE_NON_NULL(s);
        omnmFieldImpl field;
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
        VALIDATE_NON_NULL(s);
        memset (s, 0, sizeof(T));

        // If size on wire doesn't match requested size, return error
        if (field.mSize > sizeof(T))
        {
            return MAMA_STATUS_WRONG_FIELD_TYPE;
        }
        switch (field.mFieldType)
        {
            case MAMA_FIELD_TYPE_BOOL:
            {
                mama_bool_t* currentValue = (mama_bool_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_CHAR:
            {
                char* currentValue = (char*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_I8:
            {
                mama_i8_t* currentValue = (mama_i8_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_U8:
            {
                mama_u8_t* currentValue = (mama_u8_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_I16:
            {
                mama_i16_t* currentValue = (mama_i16_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_U16:
            {
                mama_u16_t* currentValue = (mama_u16_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_I32:
            {
                mama_i32_t* currentValue = (mama_i32_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_U32:
            {
                mama_u32_t* currentValue = (mama_u32_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_F32:
            {
                mama_f32_t* currentValue = (mama_f32_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_QUANTITY:
            {
                mama_quantity_t* currentValue = (mama_quantity_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_I64:
            {
                mama_i64_t* currentValue = (mama_i64_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_U64:
            {
                mama_u64_t* currentValue = (mama_u64_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            case MAMA_FIELD_TYPE_F64:
            {
                mama_f64_t* currentValue = (mama_f64_t*) field.mData;
                *s = (T)*currentValue;
                break;
            }
            default:
                memcpy (s, field.mData, field.mSize);
                break;
        }

        return MAMA_STATUS_OK;
    }

    // Templatized function for casting buffers to data types by lookup
    template<typename T>
    mama_status
    getFieldValueAsBuffer(mamaFieldType type, const char *name, mama_fid_t fid, T **s)
    {
        omnmFieldImpl field;
        VALIDATE_NAME_FID(name, fid);
        VALIDATE_NON_NULL(s);
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
        VALIDATE_NON_NULL(s);
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

    static void
    convertOmnmDateTimeToMamaDateTime (omnmDateTime* from, mamaDateTime to);

    static void
    convertMamaDateTimeToOmnmDateTime (mamaDateTime from, omnmDateTime* to);

    static void
    convertOmnmPriceToMamaPrice (omnmPrice* from, mamaPrice to);

    static void
    convertMamaPriceToOmnmPrice (mamaPrice from, omnmPrice* to);

    // Clear the payload
    mama_status
    clear ();

    // Get the number of bytes in the current header
    uint16_t
    getHeaderSize();

   mama_status
   updateSubMsg (msgPayload msg, const char* name, mama_fid_t fid, const msgPayload value);


    // Underlying buffer to store the payload
    uint8_t*      mPayloadBuffer;

    // This really reflects the capacity of mPayloadBuffermPayloadBufferTail
    size_t        mPayloadBufferSize;

    // Tail always points to the end of the 'useful' paromnt of the buffer, where
    // the free space in the remaining buffer lives
    size_t        mPayloadBufferTail;

    // Reusable field to use during payload interface crud operations
    omnmFieldImpl mField;

    // If this is a member of a mamaMsg, this is a pointer to it
    mamaMsg       mParent;

    // Meta data for the payload
    omnmHeader    mHeader;
private:
    // Find the field inside the buffer and populate provided field with its
    // location
    mama_status findFieldInBuffer (const char* name, mama_fid_t fid, struct omnmFieldImpl& field);
};

void
omnmmsgFieldPayloadImpl_cleanup (omnmFieldImpl* impl);


#endif /* MAMA_BRIDGE_OMNM_MSG_PAYLOAD_H__ */
