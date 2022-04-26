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

#ifndef MAMA_PAYLOAD_OMNMMSG_OMNMMSGIMPL_H__
#define MAMA_PAYLOAD_OMNMMSG_OMNMMSGIMPL_H__

#include <mama/integration/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_init              (mamaPayloadBridge bridge, char* identifier);

MAMAIgnoreDeprecatedOpen
MAMAExpBridgeDLL
mamaPayloadType
omnmmsgPayload_getType           (void);
MAMAIgnoreDeprecatedClose

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_create            (msgPayload*         msg);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_createForTemplate (msgPayload*         msg,
                                  mamaPayloadBridge   bridge,
                                  mama_u32_t          templateId);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_copy             (const msgPayload    msg,
                                 msgPayload*         copy);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_clear            (msgPayload          msg);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_destroy          (msgPayload          msg);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_setParent        (msgPayload          msg,
                                 const mamaMsg       parent);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getByteSize      (msgPayload          msg,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getNumFields     (const msgPayload    msg,
                                 mama_size_t*        numFields);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getSendSubject   (const msgPayload    msg,
                                 const char**        subject);

MAMAExpBridgeDLL
const char*
omnmmsgPayload_toString         (const msgPayload    msg);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_iterateFields    (const msgPayload    msg,
                                 const mamaMsg       parent,
                                 mamaMsgField        field,
                                 mamaMsgIteratorCb   cb,
                                 void*               closure);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_serialize        (const msgPayload    msg,
                                 const void**        buffer,
                                 mama_size_t*        bufferLength);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_unSerialize      (const msgPayload    msg,
                                 const void*         buffer,
                                 mama_size_t         bufferLength);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getByteBuffer    (const msgPayload    msg,
                                 const void**        buffer,
                                 mama_size_t*        bufferLength);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_setByteBuffer    (const msgPayload    msg,
                                 mamaPayloadBridge   bridge,
                                 const void*         buffer,
                                 mama_size_t         bufferLength);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_createFromByteBuffer (msgPayload*         msg,
                                     mamaPayloadBridge   bridge,
                                     const void*         buffer,
                                     mama_size_t         bufferLength);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_apply            (msgPayload          dest,
                                 const msgPayload    src);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getNativeMsg     (const msgPayload    msg,
                                 void**              nativeMsg);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getFieldAsString (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 char*               buffer,
                                 mama_size_t         len);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addBool          (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_bool_t         value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addChar          (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 char                value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addI8            (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i8_t           value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addU8            (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u8_t           value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addI16           (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i16_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addU16           (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u16_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addI32           (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i32_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addU32           (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u32_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addI64           (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i64_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addU64           (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u64_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addF32           (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_f32_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addF64           (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_f64_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addString        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const char*         value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addOpaque        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const void*         value,
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addDateTime      (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaDateTime  value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addPrice         (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaPrice     value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addMsg           (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaMsg       value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorBool    (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_bool_t   value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorChar    (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const char          value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorI8      (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i8_t     value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorU8      (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u8_t     value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorI16     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i16_t    value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorU16     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u16_t    value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorI32     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i32_t    value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorU32     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u32_t    value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorI64     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i64_t    value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorU64     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u64_t    value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorF32     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_f32_t    value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorF64     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_f64_t    value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorString  (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const char*         value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorMsg     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaMsg       value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorDateTime (msgPayload          msg,
                                  const char*         name,
                                  mama_fid_t          fid,
                                  const mamaDateTime  value[],
                                  mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_addVectorPrice   (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaPrice     value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateBool       (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_bool_t         value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateChar       (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 char                value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateI8         (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i8_t           value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateU8         (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u8_t           value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateI16        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i16_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateU16        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u16_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateI32        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i32_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateU32        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u32_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateI64        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i64_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateU64        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u64_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateF32        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_f32_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateF64        (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_f64_t          value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateString     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const char*         value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateOpaque     (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const void*         value,
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateDateTime   (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaDateTime  value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updatePrice      (msgPayload          msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaPrice     value);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateSubMsg     (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mamaMsg       subMsg);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorMsg  (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mamaMsg       value[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorString (msgPayload         msg,
                                   const char*        fname,
                                   mama_fid_t         fid,
                                   const char*        strList[],
                                   mama_size_t        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorBool (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_bool_t   boolList[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorChar (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const char          charList[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorI8   (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_i8_t     i8List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorU8   (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_u8_t     u8List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorI16  (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_i16_t    i16List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorU16  (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_u16_t    u16List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorI32  (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_i32_t    i32List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorU32  (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_u32_t    u32List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorI64  (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_i64_t    i64List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorU64  (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_u64_t    u64List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorF32  (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_f32_t    f32List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorF64  (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mama_f64_t    f64List[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorPrice (msgPayload          msg,
                                  const char*         fname,
                                  mama_fid_t          fid,
                                  const mamaPrice     priceList[],
                                  mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_updateVectorTime (msgPayload          msg,
                                 const char*         fname,
                                 mama_fid_t          fid,
                                 const mamaDateTime  timeList[],
                                 mama_size_t         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getBool          (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_bool_t*        result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getChar          (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 char*               result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getI8            (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i8_t*          result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getU8            (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u8_t*          result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getI16           (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i16_t*         result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getU16           (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u16_t*         result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getI32           (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i32_t*         result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getU32           (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u32_t*         result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getI64           (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_i64_t*         result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getU64           (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_u64_t*         result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getF32           (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_f32_t*         result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getF64           (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mama_f64_t*         result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getString        (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const char**        result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getOpaque        (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const void**        result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getDateTime      (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mamaDateTime        result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getPrice         (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 mamaPrice           result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getMsg           (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 msgPayload*         result);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorBool    (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_bool_t** result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorChar    (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const char**        result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorI8      (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i8_t**   result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorU8      (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u8_t**   result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorI16     (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i16_t**  result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorU16     (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u16_t**  result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorI32     (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i32_t**  result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorU32     (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u32_t**  result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorI64     (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_i64_t**  result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorU64     (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_u64_t**  result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorF32     (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_f32_t**  result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorF64     (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mama_f64_t**  result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorString  (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const char***       result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorDateTime (const msgPayload     msg,
                                  const char*          name,
                                  mama_fid_t           fid,
                                  const mamaDateTime** result,
                                  mama_size_t*         size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorPrice   (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const mamaPrice**   result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getVectorMsg     (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 const msgPayload**  result,
                                 mama_size_t*        size);

MAMAExpBridgeDLL
mama_status
omnmmsgPayload_getField         (const msgPayload    msg,
                                 const char*         name,
                                 mama_fid_t          fid,
                                 msgFieldPayload*    result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_create      (msgFieldPayload*        field);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_destroy     (msgFieldPayload         field);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getType     (const msgFieldPayload   field,
                                 mamaFieldType*          result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getName     (msgFieldPayload         field,
                                 mamaDictionary          dict,
                                 mamaFieldDescriptor     desc,
                                 const char**            result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getFid      (const msgFieldPayload   field,
                                 mamaDictionary          dict,
                                 mamaFieldDescriptor     desc,
                                 uint16_t*               result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getDescriptor (const msgFieldPayload  field,
                                   mamaDictionary         dict,
                                   mamaFieldDescriptor*   result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateBool  (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_bool_t             value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateChar  (msgFieldPayload         field,
                                 msgPayload              msg,
                                 char                    value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateI8    (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_i8_t               value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateU8    (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_u8_t               value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateI16   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_i16_t              value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateU16   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_u16_t              value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateI32   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_i32_t              value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateU32   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_u32_t              value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateI64   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_i64_t              value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateU64   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_u64_t              value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateF32   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_f32_t              value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateF64   (msgFieldPayload         field,
                                 msgPayload              msg,
                                 mama_f64_t              value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateString (msgFieldPayload         field,
                                  msgPayload              msg,
                                  const char*             value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateDateTime (msgFieldPayload         field,
                                    msgPayload              msg,
                                    const mamaDateTime      value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updatePrice (msgFieldPayload         field,
                                 msgPayload              msg,
                                 const mamaPrice         value);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_updateSubMsg (msgFieldPayload         field,
                                  msgPayload              msg,
                                  const msgPayload        subMsg);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getBool     (const msgFieldPayload   field,
                                 mama_bool_t*            result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getChar     (const msgFieldPayload   field,
                                 char*                   result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getI8       (const msgFieldPayload   field,
                                 mama_i8_t*              result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getU8       (const msgFieldPayload   field,
                                 mama_u8_t*              result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getI16      (const msgFieldPayload   field,
                                 mama_i16_t*             result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getU16      (const msgFieldPayload   field,
                                 mama_u16_t*            result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getI32      (const msgFieldPayload   field,
                                 mama_i32_t*             result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getU32      (const msgFieldPayload   field,
                                 mama_u32_t*             result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getI64      (const msgFieldPayload   field,
                                 mama_i64_t*             result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getU64      (const msgFieldPayload   field,
                                 mama_u64_t*             result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getF32      (const msgFieldPayload   field,
                                 mama_f32_t*             result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getF64      (const msgFieldPayload   field,
                                 mama_f64_t*             result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getString   (const msgFieldPayload   field,
                                 const char**            result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getOpaque   (const msgFieldPayload   field,
                                 const void**            result,
                                 mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getDateTime (const msgFieldPayload   field,
                                 mamaDateTime            result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getPrice    (const msgFieldPayload   field,
                                 mamaPrice               result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getMsg      (const msgFieldPayload   field,
                                 msgPayload*             result);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorBool (const msgFieldPayload   field,
                                   const mama_bool_t**     result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorChar (const msgFieldPayload   field,
                                   const char**            result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorI8   (const msgFieldPayload   field,
                                   const mama_i8_t**       result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorU8   (const msgFieldPayload   field,
                                   const mama_u8_t**       result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorI16  (const msgFieldPayload   field,
                                   const mama_i16_t**      result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorU16  (const msgFieldPayload   field,
                                   const mama_u16_t**      result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorI32  (const msgFieldPayload   field,
                                   const mama_i32_t**      result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorU32  (const msgFieldPayload   field,
                                   const mama_u32_t**      result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorI64  (const msgFieldPayload   field,
                                   const mama_i64_t**      result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorU64  (const msgFieldPayload   field,
                                   const mama_u64_t**      result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorF32  (const msgFieldPayload   field,
                                   const mama_f32_t**      result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorF64  (const msgFieldPayload   field,
                                   const mama_f64_t**      result,
                                   mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorString (const msgFieldPayload   field,
                                     const char***           result,
                                     mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorDateTime (const msgFieldPayload   field,
                                       const mamaDateTime**    result,
                                       mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorPrice (const msgFieldPayload   field,
                                    const mamaPrice**        result,
                                    mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getVectorMsg   (const msgFieldPayload   field,
                                    const msgPayload**      result,
                                    mama_size_t*            size);

MAMAExpBridgeDLL
mama_status
omnmmsgFieldPayload_getAsString (const msgFieldPayload   field,
                                 const msgPayload        msg,
                                 char*                   buffer,
                                 mama_size_t             len);

MAMAExpBridgeDLL
mama_status
omnmmsgPayloadIter_create   (msgPayloadIter*         iter,
                             msgPayload              msg);

MAMAExpBridgeDLL
msgFieldPayload
omnmmsgPayloadIter_next     (msgPayloadIter          iter,
                             msgFieldPayload         field,
                             msgPayload              msg);

MAMAExpBridgeDLL
mama_bool_t
omnmmsgPayloadIter_hasNext  (msgPayloadIter          iter,
                             msgPayload              msg);

MAMAExpBridgeDLL
msgFieldPayload
omnmmsgPayloadIter_begin    (msgPayloadIter          iter,
                             msgFieldPayload         field,
                             msgPayload              msg);

MAMAExpBridgeDLL
msgFieldPayload
omnmmsgPayloadIter_end      (msgPayloadIter          iter,
                             msgPayload              msg);

MAMAExpBridgeDLL
mama_status
omnmmsgPayloadIter_associate (msgPayloadIter          iter,
                              msgPayload              msg);

MAMAExpBridgeDLL
mama_status
omnmmsgPayloadIter_destroy   (msgPayloadIter          iter);

#if defined(__cplusplus)
}
#endif

#endif /* MAMA_PAYLOAD_OMNMMSG_OMNMMSGIMPL_H__ */
