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
#include <wombat/memnode.h>

#include "Payload.h"
#include "omnmmsgpayloadfunctions.h"
#include <wombat/strutils.h>


/*=========================================================================
  =                              Macros                                   =
  =========================================================================*/

#define EXPAND_PRINT_SCALAR_MACROS(TYPE,INITIAL,SUBSET,FORMAT,CASTTO)          \
do                                                                             \
{                                                                              \
    TYPE result = INITIAL;                                                     \
    omnmmsgFieldPayload_get##SUBSET (field, &result);                          \
    snprintf (buffer, len, FORMAT, (CASTTO)result);                            \
} while (0)

#define EXPAND_PRINT_VECTOR_MACROS(TYPE,SUBSET,FORMAT,CASTTO)                  \
do                                                                             \
{                                                                              \
    const TYPE * result = NULL;                                                \
    mama_size_t  size   = 0;                                                   \
    mama_size_t  i      = 0;                                                   \
    mama_status  status = mamaMsgField_getVector##SUBSET (                     \
                                  (const mamaMsgField) field,                  \
                                  &result,                                     \
                                  &size);                                      \
                                                                               \
    if (MAMA_STATUS_OK != status)                                              \
    {                                                                          \
        snprintf (buffer, len, "{failed with status: %d}", status);            \
        return status;                                                         \
    }                                                                          \
    snprintf (buffer, len, "{");                                               \
    for (i = 0; i < size; i++)                                                 \
    {                                                                          \
       snprintf (buffer, len, FORMAT, (CASTTO) result[i]);                     \
       snprintf (buffer, len, ",");                                            \
       if (i == (size -1))                                                     \
           snprintf (buffer, len, "\b");                                       \
    }                                                                          \
    snprintf (buffer, len, "}");                                               \
} while (0)

#define GET_SCALAR_FIELD(FIELD,RESULT,TYPE)                                    \
do                                                                             \
{                                                                              \
    omnmFieldImpl* impl = (omnmFieldImpl*)FIELD;                               \
    if (NULL == field || NULL == result) return MAMA_STATUS_NULL_ARG;          \
    if (NULL == impl->mData) return MAMA_STATUS_INVALID_ARG;                   \
    if (TYPE != impl->mFieldType &&                                            \
        false == OmnmPayloadImpl::areFieldTypesCastable(impl->mFieldType, TYPE)) \
        return MAMA_STATUS_WRONG_FIELD_TYPE;                                   \
    return impl->mParent->getFieldValueAsCopy (*impl, RESULT);                 \
} while (0)

#define UPDATE_SCALAR_FIELD(FIELD,MSG,VALUE,TYPE)                              \
do                                                                             \
{                                                                              \
    omnmFieldImpl* impl = (omnmFieldImpl*)FIELD;                               \
    if (NULL == FIELD || NULL == MSG) return MAMA_STATUS_NULL_ARG;             \
    if (TYPE != impl->mFieldType) return MAMA_STATUS_WRONG_FIELD_TYPE;         \
    return impl->mParent->updateField(TYPE,                                    \
                                      *impl,                                   \
                                      (uint8_t*)&VALUE,                        \
                                      sizeof(VALUE));                          \
} while (0)

#define GET_SCALAR_VECTOR(FIELD,RESULT,SIZE,TYPE)                              \
do                                                                             \
{                                                                              \
    mama_status status = MAMA_STATUS_OK;                                       \
    omnmFieldImpl* impl = (omnmFieldImpl*)FIELD;                               \
                                                                               \
    if (NULL == FIELD || NULL == RESULT || NULL == SIZE)                       \
        return MAMA_STATUS_NULL_ARG;                                           \
                                                                               \
    if (MAMA_STATUS_OK == status)                                              \
    {                                                                          \
        *size = impl->mSize / (sizeof(TYPE));                                  \
        *result = (TYPE*) impl->mData;                                         \
    }                                                                          \
                                                                               \
    return status;                                                             \
} while (0)

/*=========================================================================
  =                   Public interface functions                          =
  =========================================================================*/

mama_status
omnmmsgFieldPayload_create (msgFieldPayload* field)
{
    omnmFieldImpl* impl = (omnmFieldImpl*) calloc (1, sizeof(omnmFieldImpl));

    *field = (msgPayload) impl;

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgFieldPayload_destroy (msgFieldPayload field)
{
    omnmmsgFieldPayloadImpl_cleanup ((omnmFieldImpl*)field);
    free (field);
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgFieldPayload_getType (const msgFieldPayload   field,
                             mamaFieldType*          result)
{
    if (NULL == field || NULL == result) return MAMA_STATUS_NULL_ARG;
    *result = ((omnmFieldImpl*)field)->mFieldType;
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgFieldPayload_getName (msgFieldPayload         field,
                             mamaDictionary          dict,
                             mamaFieldDescriptor     desc,
                             const char**            result)
{
    omnmFieldImpl*           impl = (omnmFieldImpl*) field;
    const char*              fieldDescName = NULL;

    if (NULL == impl || NULL == result)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    /* If a name is part of field and it's not a NULL string */
    if (NULL != impl->mName && '\0' != *impl->mName)
    {
        *result = impl->mName;
    }

    /* If there is a dictionary, but no descriptor */
    if (NULL != dict && NULL == desc)
    {
        omnmmsgFieldPayload_getDescriptor (field, dict, &desc);
    }

    /* If a descriptor was provided, use it to obtain the name */
    if (NULL != desc)
    {
        /* get the name from descriptor. If that fails, move on */
        fieldDescName = mamaFieldDescriptor_getName (desc);
        if (NULL != fieldDescName)
        {
            *result = fieldDescName;
        }
    }

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgFieldPayload_getFid (const msgFieldPayload   field,
                            mamaDictionary          dict,
                            mamaFieldDescriptor     desc,
                            uint16_t*               result)
{
    if (NULL == field || NULL == result) return MAMA_STATUS_NULL_ARG;
    *result = ((omnmFieldImpl*)field)->mFid;
    return MAMA_STATUS_OK;
}

mama_status
omnmmsgFieldPayload_getDescriptor(const msgFieldPayload  field,
                                  mamaDictionary        dict,
                                  mamaFieldDescriptor*  result)
{

    omnmFieldImpl*          impl        = (omnmFieldImpl*) field;
    mamaFieldDescriptor     tmpResult   = NULL;
    mama_status             status      = MAMA_STATUS_OK;

    if (NULL == impl || NULL == result)
    {
        return MAMA_STATUS_NULL_ARG;
    }

    if (0 != impl->mFid)
    {
        status = mamaDictionary_getFieldDescriptorByFid (dict,
                                                         &tmpResult,
                                                         impl->mFid);
        if (MAMA_STATUS_OK == status)
        {
            *result = tmpResult;
        }
        return status;
    }

    if (NULL != impl->mName)
    {
        status = mamaDictionary_getFieldDescriptorByName (dict,
                                                          &tmpResult,
                                                          impl->mName);
        if (MAMA_STATUS_OK == status)
        {
            *result = tmpResult;
        }
        return status;
    }
    else
    {
        return MAMA_STATUS_INVALID_ARG;
    }
}

mama_status
omnmmsgFieldPayload_updateBool (msgFieldPayload         field,
                                msgPayload              msg,
                                mama_bool_t             value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_BOOL);
}

mama_status
omnmmsgFieldPayload_updateChar (msgFieldPayload         field,
                                msgPayload              msg,
                                char                    value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_CHAR);
}

mama_status
omnmmsgFieldPayload_updateI8    (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_i8_t               value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_I8);
}

mama_status
omnmmsgFieldPayload_updateU8    (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_u8_t               value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_U8);
}

mama_status
omnmmsgFieldPayload_updateI16   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_i16_t              value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_I16);
}

mama_status
omnmmsgFieldPayload_updateU16   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_u16_t              value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_U16);
}

mama_status
omnmmsgFieldPayload_updateI32   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_i32_t              value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_I32);
}

mama_status
omnmmsgFieldPayload_updateU32   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_u32_t              value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_U32);
}

mama_status
omnmmsgFieldPayload_updateI64   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_i64_t              value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_I64);
}

mama_status
omnmmsgFieldPayload_updateU64   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_u64_t              value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_U64);
}

mama_status
omnmmsgFieldPayload_updateF32   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_f32_t              value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_F32);
}

mama_status
omnmmsgFieldPayload_updateF64   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_f64_t              value)
{
    UPDATE_SCALAR_FIELD(field, msg, value, MAMA_FIELD_TYPE_F64);
}

mama_status
omnmmsgFieldPayload_updateString(msgFieldPayload         field,
                                 msgPayload              msg,
                                 const char*             str)
{
    omnmFieldImpl* impl = (omnmFieldImpl*) field;
    if (NULL == impl || NULL == msg) return MAMA_STATUS_NULL_ARG;
    return impl->mParent->updateField (MAMA_FIELD_TYPE_STRING,
                                       *impl,
                                       (uint8_t*)str,
                                       strlenEx(str) + 1);
}

mama_status
omnmmsgFieldPayload_updateDateTime
                               (msgFieldPayload         field,
                                msgPayload              msg,
                                const mamaDateTime      value)
{
    if (NULL == field || NULL == msg || NULL == value) return MAMA_STATUS_NULL_ARG;

    OmnmPayloadImpl* impl = (OmnmPayloadImpl*)msg;
    omnmDateTime dateTime;

    OmnmPayloadImpl::convertMamaDateTimeToOmnmDateTime(value, &dateTime);

    return impl->updateField (MAMA_FIELD_TYPE_TIME,
                              *((omnmFieldImpl*)field),
                              (uint8_t*)&dateTime,
                              sizeof(dateTime));
}

mama_status
omnmmsgFieldPayload_updatePrice (msgFieldPayload         field,
                                 msgPayload              msg,
                                 const mamaPrice         value)
{
    if (NULL == field || NULL == msg || NULL == value) return MAMA_STATUS_NULL_ARG;
    OmnmPayloadImpl* impl = (OmnmPayloadImpl*)msg;
    omnmPrice price;

    OmnmPayloadImpl::convertMamaPriceToOmnmPrice(value, &price);

    return impl->updateField(MAMA_FIELD_TYPE_PRICE,
        *((omnmFieldImpl*)field),
        (uint8_t*)&price,
        sizeof(price));

}

mama_status
omnmmsgFieldPayload_updateSubMsg(msgFieldPayload         field,
                                 msgPayload              msg,
                                 const msgPayload        subMsg)
{
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

mama_status
omnmmsgFieldPayload_getBool (const msgFieldPayload   field,
                             mama_bool_t*            result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_BOOL);
}

mama_status
omnmmsgFieldPayload_getChar (const msgFieldPayload   field,
                             char*                   result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_CHAR);
}

mama_status
omnmmsgFieldPayload_getI8   (const msgFieldPayload   field,
                             mama_i8_t*              result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_I8);
}

mama_status
omnmmsgFieldPayload_getU8   (const msgFieldPayload   field,
                             mama_u8_t*              result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_U8);
}

mama_status
omnmmsgFieldPayload_getI16  (const msgFieldPayload   field,
                             mama_i16_t*             result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_I16);
}

mama_status
omnmmsgFieldPayload_getU16  (const msgFieldPayload   field,
                             mama_u16_t*            result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_U16);
}

mama_status
omnmmsgFieldPayload_getI32  (const msgFieldPayload   field,
                             mama_i32_t*             result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_I32);
}

mama_status
omnmmsgFieldPayload_getU32  (const msgFieldPayload   field,
                             mama_u32_t*             result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_U32);
}

mama_status
omnmmsgFieldPayload_getI64  (const msgFieldPayload   field,
                             mama_i64_t*             result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_I64);
}

mama_status
omnmmsgFieldPayload_getU64  (const msgFieldPayload   field,
                             mama_u64_t*             result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_U64);
}

mama_status
omnmmsgFieldPayload_getF32  (const msgFieldPayload   field,
                             mama_f32_t*             result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_F32);
}

mama_status
omnmmsgFieldPayload_getF64  (const msgFieldPayload   field,
                             mama_f64_t*             result)
{
    GET_SCALAR_FIELD (field, result, MAMA_FIELD_TYPE_F64);
}

mama_status
omnmmsgFieldPayload_getString (const msgFieldPayload   field,
                               const char**            result)
{
    omnmFieldImpl* impl = (omnmFieldImpl*)field;
    if (NULL == field || NULL == result) return MAMA_STATUS_NULL_ARG;
    if (NULL == impl->mData) return MAMA_STATUS_INVALID_ARG;
    if (MAMA_FIELD_TYPE_STRING != impl->mFieldType &&
        false == OmnmPayloadImpl::areFieldTypesCastable(impl->mFieldType, MAMA_FIELD_TYPE_STRING))
    {
        return MAMA_STATUS_WRONG_FIELD_TYPE;
    }

    return impl->mParent->getFieldValueAsBuffer (*impl, result);
}

mama_status
omnmmsgFieldPayload_getOpaque (const msgFieldPayload   field,
                               const void**            result,
                               mama_size_t*            size)
{
    omnmFieldImpl* impl = (omnmFieldImpl*)field;
    if (NULL == field || NULL == result) return MAMA_STATUS_NULL_ARG;
    if (NULL == impl->mData) return MAMA_STATUS_INVALID_ARG;
    if (MAMA_FIELD_TYPE_OPAQUE != impl->mFieldType &&
        false == OmnmPayloadImpl::areFieldTypesCastable(impl->mFieldType, MAMA_FIELD_TYPE_OPAQUE))
    {
        return MAMA_STATUS_WRONG_FIELD_TYPE;
    }

    mama_status status = impl->mParent->getFieldValueAsBuffer (*impl, result);
    if (status == MAMA_STATUS_OK) {
       *size = impl->mSize;
    }

    return status;
}

/*
 * getDateTime is expected to be able to handle casting from strings,
 * F64, I64, U64 and Date Time input data types.
 */
mama_status
omnmmsgFieldPayload_getDateTime (const msgFieldPayload   field,
                                 mamaDateTime            result)
{
    if (NULL == field || NULL == result) return MAMA_STATUS_NULL_ARG;
    omnmFieldImpl* impl = (omnmFieldImpl*) field;
    mama_status status = MAMA_STATUS_OK;
    switch (impl->mFieldType)
    {
    case MAMA_FIELD_TYPE_TIME:
    {
        OmnmPayloadImpl::convertOmnmDateTimeToMamaDateTime((omnmDateTime*)impl->mData, result);
        break;
    }
    case MAMA_FIELD_TYPE_STRING:
    {
        status = mamaDateTime_setFromString (result, (const char*)impl->mData);
        break;
    }
    case MAMA_FIELD_TYPE_F64:
    {
        status = mamaDateTime_setEpochTimeF64 (result, *((mama_f64_t*)impl->mData));
        break;
    }
    case MAMA_FIELD_TYPE_I64:
    {
        status = mamaDateTime_setEpochTimeMilliseconds (result, *((mama_i64_t*)impl->mData));
        break;
    }
    case MAMA_FIELD_TYPE_U64:
    {
        status = mamaDateTime_setEpochTimeMicroseconds (result, *((mama_u64_t*)impl->mData));
        break;
    }
    default:
        return MAMA_STATUS_WRONG_FIELD_TYPE;
        break;
    }
    return status;
}

mama_status
omnmmsgFieldPayload_getPrice (const msgFieldPayload   field,
                              mamaPrice               result)
{
    omnmFieldImpl* impl = (omnmFieldImpl*)field;
    if (NULL == field || NULL == result) return MAMA_STATUS_NULL_ARG;
    if (NULL == impl->mData) return MAMA_STATUS_INVALID_ARG;

    switch(impl->mFieldType)
    {
        case MAMA_FIELD_TYPE_PRICE:
        {
            OmnmPayloadImpl::convertOmnmPriceToMamaPrice((omnmPrice*)impl->mData, result);
            return MAMA_STATUS_OK;
            break;
        }
        case MAMA_FIELD_TYPE_F64:
        {
            mama_f64_t value = 0;
            mama_status status = omnmmsgFieldPayload_getF64 (field, &value);
            if (MAMA_STATUS_OK != status) return status;
            mamaPrice_setValue (result, value);
            return MAMA_STATUS_OK;
            break;
        }
        default:
            return MAMA_STATUS_WRONG_FIELD_TYPE;
            break;
    }
}

mama_status
omnmmsgFieldPayload_getMsg (const msgFieldPayload   field,
                            msgPayload*             result)
{
    mama_status status;
    omnmFieldImpl* impl = (omnmFieldImpl*)field;
    if (NULL == field || NULL == result) return MAMA_STATUS_NULL_ARG;
    if (NULL == impl->mData) return MAMA_STATUS_INVALID_ARG;
    if (MAMA_FIELD_TYPE_MSG != impl->mFieldType) return MAMA_STATUS_WRONG_FIELD_TYPE;

    if (NULL == impl->mSubPayload)
    {
        status = omnmmsgPayload_createFromByteBuffer (&impl->mSubPayload,
                                                      NULL,
                                                      impl->mData,
                                                      impl->mSize);
    }
    else
    {
        status = omnmmsgPayload_setByteBuffer (impl->mSubPayload,
                                               NULL,
                                               impl->mData,
                                               impl->mSize);
    }
    *result = impl->mSubPayload;
    return status;
}

mama_status
omnmmsgFieldPayload_getVectorBool (const msgFieldPayload   field,
                                   const mama_bool_t**     result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_bool_t);
}

mama_status
omnmmsgFieldPayload_getVectorChar (const msgFieldPayload   field,
                                   const char**            result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, char);
}

mama_status
omnmmsgFieldPayload_getVectorI8   (const msgFieldPayload   field,
                                   const mama_i8_t**       result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_i8_t);
}

mama_status
omnmmsgFieldPayload_getVectorU8   (const msgFieldPayload   field,
                                   const mama_u8_t**       result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_u8_t);
}

mama_status
omnmmsgFieldPayload_getVectorI16  (const msgFieldPayload   field,
                                   const mama_i16_t**      result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_i16_t);
}

mama_status
omnmmsgFieldPayload_getVectorU16  (const msgFieldPayload   field,
                                   const mama_u16_t**      result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_u16_t);
}

mama_status
omnmmsgFieldPayload_getVectorI32  (const msgFieldPayload   field,
                                   const mama_i32_t**      result,
                                   mama_size_t*            size)
{

    GET_SCALAR_VECTOR (field, result, size, mama_i32_t);
}

mama_status
omnmmsgFieldPayload_getVectorU32  (const msgFieldPayload   field,
                                   const mama_u32_t**      result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_u32_t);
}

mama_status
omnmmsgFieldPayload_getVectorI64  (const msgFieldPayload   field,
                                   const mama_i64_t**      result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_i64_t);
}

mama_status
omnmmsgFieldPayload_getVectorU64  (const msgFieldPayload   field,
                                   const mama_u64_t**      result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_u64_t);
}

mama_status
omnmmsgFieldPayload_getVectorF32  (const msgFieldPayload   field,
                                   const mama_f32_t**      result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_f32_t);
}

mama_status
omnmmsgFieldPayload_getVectorF64  (const msgFieldPayload   field,
                                   const mama_f64_t**      result,
                                   mama_size_t*            size)
{
    GET_SCALAR_VECTOR (field, result, size, mama_f64_t);
}

mama_status
omnmmsgFieldPayload_getVectorString (const msgFieldPayload   field,
                                     const char***           result,
                                     mama_size_t*            size)
{
    omnmFieldImpl* impl = (omnmFieldImpl*)field;
    size_t stringCount = 0, i = 0, j=0;

    VALIDATE_NON_NULL(field);
    VALIDATE_NON_NULL(result);
    VALIDATE_NON_NULL(size);

    for(i = 0; i < impl->mSize; i++)
    {
        if (((char*)impl->mData)[i] == '\0')
            stringCount++;
    }

    *size = stringCount;

    // Ensure the buffer is big enough for this
    allocateBufferMemory ((void**)&impl->mVectorString,
                          (size_t*)&impl->mVectorStringLen,
                          sizeof(char*) * stringCount);

    /* NB - i++ will add null character on each iteration */
    for(i = 0; i < impl->mSize; i++)
    {
        impl->mVectorString[j] = ((char*)impl->mData) + i;
        i += strlen(impl->mVectorString[j]);
        j++;
    }

    *result = impl->mVectorString;

    return MAMA_STATUS_OK;
}

/*
 * Postponing implementation until this type of vectors has a standard protocol
 * or is removed from the implementation
 */
mama_status
omnmmsgFieldPayload_getVectorDateTime (const msgFieldPayload   field,
                                       const mamaDateTime**    result,
                                       mama_size_t*            size)
{
    omnmFieldImpl* impl = (omnmFieldImpl*)field;
    omnmDateTime* rawDateTimes;
    size_t count = 0, i = 0;

    VALIDATE_NON_NULL(field);
    VALIDATE_NON_NULL(result);
    VALIDATE_NON_NULL(size);

    count = impl->mSize / sizeof(omnmDateTime);

    // Ensure the buffer is big enough for this
    allocateBufferMemory((void**)&impl->mVectorDateTime,
        (size_t*)&impl->mVectorDateTimeLen,
        sizeof(char*) * count);

    rawDateTimes = (omnmDateTime*)impl->mData;
    /* NB - i++ will add null character on each iteration */
    for (i = 0; i < count; i++)
    {
        if (NULL == impl->mVectorDateTime[i])
        {
            mamaDateTime_create(&impl->mVectorDateTime[i]);

        }
        else
        {
            mamaDateTime_clear(impl->mVectorDateTime[i]);
        }

        OmnmPayloadImpl::convertOmnmDateTimeToMamaDateTime(&rawDateTimes[i], impl->mVectorDateTime[i]);
    }

    *result = impl->mVectorDateTime;
    *size = count;

    return MAMA_STATUS_OK;
}

mama_status
omnmmsgFieldPayload_getVectorPrice    (const msgFieldPayload   field,
                                       const mamaPrice**       result,
                                       mama_size_t*            size)
{
    omnmFieldImpl* impl = (omnmFieldImpl*)field;
    omnmPrice* rawPrices;
    size_t count = 0, i = 0;

    VALIDATE_NON_NULL(field);
    VALIDATE_NON_NULL(result);
    VALIDATE_NON_NULL(size);

    count = impl->mSize / sizeof(omnmPrice);

    // Ensure the buffer is big enough for this
    allocateBufferMemory((void**)&impl->mVectorPrice,
        (size_t*)&impl->mVectorPriceLen,
        sizeof(char*) * count);

    rawPrices = (omnmPrice*)impl->mData;
    /* NB - i++ will add null character on each iteration */
    for (i = 0; i < count; i++)
    {
        if (NULL == impl->mVectorPrice[i])
        {
            mamaPrice_create(&impl->mVectorPrice[i]);

        }
        else
        {
            mamaPrice_clear(impl->mVectorPrice[i]);
        }

        OmnmPayloadImpl::convertOmnmPriceToMamaPrice(&rawPrices[i], impl->mVectorPrice[i]);
    }

    *result = impl->mVectorPrice;
    *size = count;

    return MAMA_STATUS_OK;
}


mama_status
omnmmsgFieldPayload_getVectorMsg      (const msgFieldPayload   field,
                                       const msgPayload**      result,
                                       mama_size_t*            size)
{
    omnmFieldImpl* impl = (omnmFieldImpl*)field;
    size_t msgCount = 0, i = 0, j = 0;
    mama_status status = MAMA_STATUS_OK;

    VALIDATE_NON_NULL(field);
    VALIDATE_NON_NULL(result);
    VALIDATE_NON_NULL(size);

    for(i = 0; i < impl->mSize;)
    {
        mama_size_t payloadLen = (mama_size_t) *(mama_u32_t*)impl->mData;
        i += payloadLen + sizeof(mama_u32_t);
        msgCount++;
    }

    *size = msgCount;

    // Ensure the buffer is big enough for this
    allocateBufferMemory ((void**)&impl->mVectorPayload,
                          (size_t*)&impl->mVectorPayloadLen,
                          sizeof(char*) * msgCount);

    /* NB - i++ will add null character on each iteration */
    for(i = 0; i < impl->mSize;)
    {
        mama_size_t payloadLen = (mama_size_t) *(mama_u32_t*)impl->mData;
        uint8_t* payload = ((uint8_t*) impl->mData) + i + sizeof(mama_u32_t);

        if (NULL == impl->mVectorPayload[j])
        {
            status = omnmmsgPayload_createFromByteBuffer (&impl->mVectorPayload[j],
                                                          NULL,
                                                          payload,
                                                          payloadLen);
        }
        else
        {
            status = omnmmsgPayload_setByteBuffer (impl->mVectorPayload[j],
                                                   NULL,
                                                   payload,
                                                   payloadLen);
        }

        i += payloadLen + sizeof(mama_u32_t);
        j++;
    }

    *result = impl->mVectorPayload;

    return status;
}

mama_status
omnmmsgFieldPayload_getAsString       (const msgFieldPayload   field,
                                       const msgPayload        msg,
                                       char*                   buffer,
                                       mama_size_t             len)
{
    mama_status status = MAMA_STATUS_OK;
    omnmFieldImpl* impl = (omnmFieldImpl*)field;

    if (NULL == buffer || NULL == field)
    {
        return MAMA_STATUS_NULL_ARG;
    }
    if (0 == len)
    {
        return MAMA_STATUS_INVALID_ARG;
    }

    switch (impl->mFieldType)
    {
    case MAMA_FIELD_TYPE_BOOL:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_bool_t,
                                    (mama_bool_t) 0,
                                    Bool,
                                    "%u",
                                    mama_bool_t);
        break;
    }
    case MAMA_FIELD_TYPE_CHAR:
    {
        EXPAND_PRINT_SCALAR_MACROS (char, '\0', Char, "%c", char);
        break;
    }
    case MAMA_FIELD_TYPE_I8:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_i8_t, 0, I8, "%d", mama_i8_t);
        break;
    }
    case MAMA_FIELD_TYPE_U8:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_u8_t, 0, U8, "%u", mama_u8_t);
        break;
    }
    case MAMA_FIELD_TYPE_I16:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_i16_t, 0, I16, "%d", mama_i16_t);
        break;
    }
    case MAMA_FIELD_TYPE_U16:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_u16_t, 0, U16, "%u", mama_u16_t);
        break;
    }
    case MAMA_FIELD_TYPE_I32:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_i32_t, 0, I32, "%d", mama_i32_t);
        break;
    }
    case MAMA_FIELD_TYPE_U32:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_u32_t, 0, U32, "%u", mama_u32_t);
        break;
    }
    case MAMA_FIELD_TYPE_I64:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_i64_t,
                                    0,
                                    I64,
                                    "%lld",
                                    long long);
        break;
    }
    case MAMA_FIELD_TYPE_U64:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_u64_t,
                                    0,
                                    U64,
                                    "%llu",
                                    long long unsigned);
        break;
    }
    case MAMA_FIELD_TYPE_F32:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_f32_t,
                                    (mama_f32_t)0.0,
                                    F32,
                                    "%f",
                                    mama_f32_t);
        break;
    }
    case MAMA_FIELD_TYPE_F64:
    {
        EXPAND_PRINT_SCALAR_MACROS (mama_f64_t,
                                    (mama_f64_t)0.0,
                                    F64,
                                    "%f",
                                    mama_f64_t);
        break;
    }
    case MAMA_FIELD_TYPE_STRING:
    {
        EXPAND_PRINT_SCALAR_MACROS (const char*, "", String, "%s", const char*);
        break;
    }
    case MAMA_FIELD_TYPE_TIME:
    {
        mamaDateTime result = NULL;
        char         dateTimeString[56];

        mamaDateTime_create (&result);
        omnmmsgFieldPayload_getDateTime (field, result);

        status = mamaDateTime_getAsString (result, dateTimeString, 56);
        snprintf (buffer, len, "%s", dateTimeString);

        mamaDateTime_destroy (result);
        break;
    }
    case MAMA_FIELD_TYPE_PRICE:
    {
        mamaPrice   result = NULL;
        char        priceString[56];

        mamaPrice_create             (&result);
        omnmmsgFieldPayload_getPrice (field, result);

        status = mamaPrice_getAsString (result, priceString, 56);
        snprintf (buffer, len, "%s", priceString);

        mamaPrice_destroy (result);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_STRING:
    {
        EXPAND_PRINT_VECTOR_MACROS (char*, String, "%s", const char*);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_U8:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_u8_t, U8, "%u", mama_u8_t);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_U16:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_u16_t, U16, "%u", mama_u16_t);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_U32:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_u32_t, U32, "%u", mama_u32_t);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_U64:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_u64_t,
                                    U64,
                                    "%llu",
                                    long long unsigned);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_BOOL:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_bool_t, Bool, "%u", mama_bool_t);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_CHAR:
    {
        EXPAND_PRINT_VECTOR_MACROS (char, Char, "%c", char);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_I8:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_i8_t, I8, "%d", mama_i8_t);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_I16:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_i16_t, I16, "%d", mama_i16_t);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_I32:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_i32_t, I32, "%d", mama_i32_t);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_I64:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_i64_t, I64, "%lld", long long);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_F64:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_f64_t, F64, "%f", mama_f64_t);
        break;
    }
    case MAMA_FIELD_TYPE_VECTOR_F32:
    {
        EXPAND_PRINT_VECTOR_MACROS (mama_f32_t, F32, "%f", mama_f32_t);
        break;
    }
    case MAMA_FIELD_TYPE_OPAQUE:
    {
        const void* result   = NULL;
        mama_size_t dataSize = 0;
        status = omnmmsgFieldPayload_getOpaque (field, &result, &dataSize);
        sprintf(buffer, "{%ld bytes}", dataSize);
        break;
    }
    case MAMA_FIELD_TYPE_MSG:
    case MAMA_FIELD_TYPE_VECTOR_MSG:
    case MAMA_FIELD_TYPE_VECTOR_TIME:
    case MAMA_FIELD_TYPE_VECTOR_PRICE:
    {
        snprintf (buffer, len, "{...}");
        break;
    }
    default:
    {
        snprintf (buffer, len, "Unknown");
        break;
    }
    }
    return status;
}

/*=========================================================================
  =                   Public implementation functions                     =
  =========================================================================*/

mama_status
omnmmsgFieldPayload_setParent         (const msgFieldPayload   field,
                                       const msgPayload        parent)
{
    ((omnmFieldImpl*)field)->mParent = (OmnmPayloadImpl*) parent;
    return MAMA_STATUS_NOT_IMPLEMENTED;
}

void
omnmmsgFieldPayloadImpl_cleanup (omnmFieldImpl* impl)
{
    mama_size_t i = 0, count = 0;

    /* Complex data type elements below */
    if (NULL != impl->mBuffer)
    {
        free (impl->mBuffer);
        impl->mBuffer = NULL;
    }
    if (NULL != impl->mSubPayload)
    {
        omnmmsgPayload_destroy (impl->mSubPayload);
        impl->mSubPayload = NULL;
    }
    if (NULL != impl->mVectorString)
    {
        free (impl->mVectorString);
        impl->mVectorString = NULL;
    }
    if (NULL != impl->mVectorPayload)
    {
        count = impl->mVectorPayloadLen / sizeof(msgPayload);
        for (i = 0; i < count; i++)
        {
            omnmmsgPayload_destroy (impl->mVectorPayload[i]);
        }
        free (impl->mVectorPayload);
        impl->mVectorPayload = NULL;
    }
    if (NULL != impl->mVectorDateTime)
    {
        count = impl->mVectorDateTimeLen / sizeof(mamaDateTime);
        for (i = 0; i < count; i++)
        {
            mamaDateTime_destroy (impl->mVectorDateTime[i]);
        }
        free (impl->mVectorDateTime);
        impl->mVectorDateTime = NULL;
    }
    if (NULL != impl->mVectorPrice)
    {
        count = impl->mVectorPriceLen / sizeof(mamaPrice);
        for (i = 0; i < count; i++)
        {
            mamaPrice_destroy (impl->mVectorPrice[i]);
        }
        free (impl->mVectorPrice);
        impl->mVectorPrice = NULL;
    }
}


