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

#include <gtest/gtest.h>
#include <mama/mama.h>
#include "omnmmsgpayloadfunctions.h"
#include "Payload.h"
#include "Iterator.h"

void parseField (const mamaMsg       msg,
                 const mamaMsgField  field,
                 void*               closure)
{
    mama_fid_t fid;
    mamaMsgField_getFid(field, &fid);
}

int main (int argc, char **argv)
{
    ::testing::InitGoogleTest (&argc, argv);
    return RUN_ALL_TESTS();
}

class OmnmTests: public ::testing::Test
{
protected:
    OmnmTests ()
    {
    }

    virtual ~OmnmTests ()
    {
    }

    virtual void SetUp ()
    {
        // Code here will be called immediately after the constructor (right
        // before each test).
        omnmmsgPayload_create (&mPayloadBase);
        mama_setLogLevel(MAMA_LOG_LEVEL_NORMAL);
    }

    virtual void TearDown ()
    {
        // Code here will be called immediately after each test (right
        // before the destructor).
        omnmmsgPayload_destroy (mPayloadBase);
    }
    mamaPayloadBridge mPayloadBridge;
    char mPayloadBridgeId;
    msgPayload mPayloadBase;
};

TEST_F(OmnmTests, CreatePayloadWithNatives)
{
    mama_i64_t actual = 0;
    mama_i64_t expected = 12398;
    omnmmsgPayload_addI64 (mPayloadBase, "banana", 666, 12398);
    omnmmsgPayload_getI64 (mPayloadBase, "banana", 666, &actual);
    ASSERT_EQ (expected, actual);
}

TEST_F(OmnmTests, CreatePayloadWithNativeFloats)
{
    mama_f64_t actual = 0;
    mama_f64_t expected = 5.46;
    omnmmsgPayload_addF64 (mPayloadBase, NULL, 666, expected);
    omnmmsgPayload_getF64 (mPayloadBase, NULL, 666, &actual);
    ASSERT_EQ (expected, actual);
}

TEST_F(OmnmTests, UpdatePayloadDateTime)
{
    mamaDateTime actual;
    mamaDateTime expected;
    mamaDateTime_create(&expected);
    mamaDateTime_setToNow(expected);
    mamaDateTime_create(&actual);
    omnmmsgPayload_addDateTime (mPayloadBase, NULL, 666, expected);
    mamaDateTime_setToMidnightToday(expected, NULL);
    omnmmsgPayload_updateDateTime (mPayloadBase, NULL, 666, expected);
    omnmmsgPayload_getDateTime (mPayloadBase, NULL, 666, actual);
    mamaDateTime_equal(expected, actual);
    mamaDateTime_destroy(expected);
    mamaDateTime_destroy(actual);
}

TEST_F(OmnmTests, CreatePayloadWithNativeStrings)
{
    const char* actual = NULL;
    const char* expected = "teststr";
    omnmmsgPayload_addString (mPayloadBase, NULL, 666, expected);
    mama_status status = omnmmsgPayload_getString (mPayloadBase, NULL, 666, &actual);
    ASSERT_EQ(MAMA_STATUS_OK, status);
    ASSERT_STREQ (expected, actual);
}

TEST_F(OmnmTests, CreatePayloadWithNativeDateTimes)
{
    mamaDateTime actual;
    mamaDateTime expected;
    mamaDateTime_create(&expected);
    mamaDateTime_setToNow(expected);
    mamaDateTime_create(&actual);
    omnmmsgPayload_addDateTime (mPayloadBase, NULL, 666, expected);
    omnmmsgPayload_getDateTime (mPayloadBase, NULL, 666, actual);
    mamaDateTime_equal(expected, actual);
    mamaDateTime_destroy(expected);
    mamaDateTime_destroy(actual);
}

TEST_F(OmnmTests, SerializeDeserialize)
{
    msgPayload copy = NULL;
    mama_i64_t actual = 0;
    mama_i64_t expected = 12398;
    const void* serializedBuffer = NULL;
    mama_size_t serializedBufferSize = 0;
    omnmmsgPayload_addI64 (mPayloadBase, "banana", 666, expected);
    omnmmsgPayload_serialize (mPayloadBase, &serializedBuffer, &serializedBufferSize);

    omnmmsgPayload_create (&copy);
    omnmmsgPayload_unSerialize (copy, (const void**)serializedBuffer, serializedBufferSize);
    omnmmsgPayload_getI64 (copy, "banana", 666, &actual);
    ASSERT_EQ (expected, actual);
    omnmmsgPayload_destroy (copy);
}

TEST_F(OmnmTests, CreatePayloadWithMultipleNatives)
{
    mama_fid_t fid1 = 100;
    mama_i64_t actual1 = 0;
    mama_i64_t expected1 = 12398;
    mama_fid_t fid2 = 101;
    mama_i64_t actual2 = 0;
    mama_i64_t expected2 = 12399;
    mama_fid_t fid3 = 102;
    const char* actual3 = "";
    const char* expected3 = "teststr";
    omnmmsgPayload_addI64 (mPayloadBase, "h", fid1, expected1);
    omnmmsgPayload_addString (mPayloadBase, NULL, fid3, expected3);
    omnmmsgPayload_addI64 (mPayloadBase, "y", fid2, expected2);

    omnmmsgPayload_getI64 (mPayloadBase, "y", fid2, &actual2);
    omnmmsgPayload_getI64 (mPayloadBase, "h", fid1, &actual1);
    omnmmsgPayload_getString (mPayloadBase, NULL, fid3, &actual3);
    ASSERT_EQ (expected1, actual1);
    ASSERT_EQ (expected2, actual2);
    ASSERT_STREQ (expected3, actual3);
}

TEST_F(OmnmTests, IteratorTest)
{
    omnmIterImpl iter;
    msgPayloadIter iterOpaque = (msgPayloadIter)&iter;

    msgFieldPayload field;
    mama_fid_t fid1 = 100;
    mama_i64_t actual1 = 0;
    mama_i64_t expected1 = 12398;

    mama_fid_t fid2 = 101;
    mama_i64_t actual2 = 0;
    mama_i64_t expected2 = 12399;

    mama_fid_t fid3 = 102;
    const char* actual3 = "";
    const char* expected3 = "teststr";

    mama_fid_t fid4 = 103;
    const char* name4 = "opaque_field1";
    const char* expected4 = "opaque value 1";

    mama_fid_t fidactual = 0;
    mamaFieldType typeactual;
    const char* nameactual;

    omnmmsgPayload_addI64 (mPayloadBase, NULL, fid1, expected1);
    omnmmsgPayload_addI64 (mPayloadBase, NULL, fid2, expected2);
    omnmmsgPayload_addString (mPayloadBase, NULL, fid3, expected3);
    omnmmsgPayload_addOpaque (mPayloadBase, name4, fid4, expected4, strlen(expected4));

    omnmmsgPayloadIterImpl_init (&iter, (OmnmPayloadImpl*)mPayloadBase);

    // field # 1 fid=100 type=int64 value=12398
    field = omnmmsgPayloadIter_next(iterOpaque, NULL, mPayloadBase);
    omnmmsgFieldPayload_getFid (field, NULL, NULL, &fidactual);
    omnmmsgFieldPayload_getType (field, &typeactual);
    omnmmsgFieldPayload_getI64 (field, &actual1);
    ASSERT_EQ (MAMA_FIELD_TYPE_I64, typeactual);
    ASSERT_EQ (fid1, fidactual);
    ASSERT_EQ (expected1, actual1);

    // field # 2 fid=101 type=int64 value=12399
    field = omnmmsgPayloadIter_next(iterOpaque, NULL, mPayloadBase);
    omnmmsgFieldPayload_getFid (field, NULL, NULL, &fidactual);
    omnmmsgFieldPayload_getType (field, &typeactual);
    omnmmsgFieldPayload_getI64 (field, &actual2);
    ASSERT_EQ (MAMA_FIELD_TYPE_I64, typeactual);
    ASSERT_EQ (fid2, fidactual);
    ASSERT_EQ (expected2, actual2);

    // Try an update too while we're here...
    expected2 = 242355;
    omnmmsgFieldPayload_updateI64 (field, mPayloadBase, expected2);
    omnmmsgFieldPayload_getI64 (field, &actual2);
    ASSERT_EQ (expected2, actual2);
    char buf[100];
    omnmmsgFieldPayload_getAsString (field, mPayloadBase, buf, 100);
    ASSERT_STREQ(buf, "242355");

    // field # 3 fid=102 type=string value="teststr"
    field = omnmmsgPayloadIter_next(iterOpaque, NULL, mPayloadBase);
    omnmmsgFieldPayload_getFid (field, NULL, NULL, &fidactual);
    omnmmsgFieldPayload_getType (field, &typeactual);
    omnmmsgFieldPayload_getString (field, &actual3);
    ASSERT_EQ (MAMA_FIELD_TYPE_STRING, typeactual);
    ASSERT_EQ (fid3, fidactual);
    ASSERT_STREQ (expected3, actual3);

    // field # 4 name="opaque_field1" fid=103 type=opaque value="opaque value 1"
    field = omnmmsgPayloadIter_next(iterOpaque, NULL, mPayloadBase);
    omnmmsgFieldPayload_getName (field, NULL, NULL, &nameactual);
    omnmmsgFieldPayload_getFid (field, NULL, NULL, &fidactual);
    omnmmsgFieldPayload_getType (field, &typeactual);
    const void* buf1;
    mama_size_t buf1Len;
    omnmmsgFieldPayload_getOpaque (field, (const void**) &buf1, &buf1Len);
    char buf2[100];
    omnmmsgFieldPayload_getAsString (field, mPayloadBase, buf2, sizeof(buf2));
    ASSERT_EQ (MAMA_FIELD_TYPE_OPAQUE, typeactual);
    ASSERT_EQ (fid4, fidactual);
    ASSERT_STREQ (name4, nameactual);
    ASSERT_STREQ (expected4, (const char*) buf1);
    //ASSERT_STREQ (expected4, buf2);

    // no more fields
    field = omnmmsgPayloadIter_next(iterOpaque, NULL, mPayloadBase);
    ASSERT_EQ (NULL, field);

    // so far, so good
    // now serialize and unserialize and try again...
    const void* serializedBuffer = NULL;
    mama_size_t serializedBufferSize = 0;
    omnmmsgPayload_serialize (mPayloadBase, &serializedBuffer, &serializedBufferSize);
    msgPayload copy = NULL;
    omnmmsgPayload_create (&copy);
    omnmmsgPayload_unSerialize (copy, (const void**)serializedBuffer, serializedBufferSize);

    omnmIterImpl iter2;
    msgPayloadIter iterOpaque2 = (msgPayloadIter)&iter2;
    omnmmsgPayloadIterImpl_init (&iter2, (OmnmPayloadImpl*)copy);

    // field # 1 fid=100 type=int64 value=12398
    field = omnmmsgPayloadIter_next(iterOpaque2, NULL, copy);
    omnmmsgFieldPayload_getFid (field, NULL, NULL, &fidactual);
    omnmmsgFieldPayload_getType (field, &typeactual);
    omnmmsgFieldPayload_getI64 (field, &actual1);
    ASSERT_EQ (MAMA_FIELD_TYPE_I64, typeactual);
    ASSERT_EQ (fid1, fidactual);
    ASSERT_EQ (expected1, actual1);

    // field # 2 fid=101 type=int64 value=12399
    field = omnmmsgPayloadIter_next(iterOpaque2, NULL, copy);
    omnmmsgFieldPayload_getFid (field, NULL, NULL, &fidactual);
    omnmmsgFieldPayload_getType (field, &typeactual);
    omnmmsgFieldPayload_getI64 (field, &actual2);
    ASSERT_EQ (MAMA_FIELD_TYPE_I64, typeactual);
    ASSERT_EQ (fid2, fidactual);
    ASSERT_EQ (expected2, actual2);
    omnmmsgFieldPayload_getAsString (field, mPayloadBase, buf, sizeof(buf));
    ASSERT_STREQ(buf, "242355");

    // field # 3 fid=102 type=string value="teststr"
    field = omnmmsgPayloadIter_next(iterOpaque2, NULL, copy);
    omnmmsgFieldPayload_getFid (field, NULL, NULL, &fidactual);
    omnmmsgFieldPayload_getType (field, &typeactual);
    omnmmsgFieldPayload_getString (field, &actual3);
    ASSERT_EQ (MAMA_FIELD_TYPE_STRING, typeactual);
    ASSERT_EQ (fid3, fidactual);
    ASSERT_STREQ (expected3, actual3);

    // field # 4 name="opaque_field1" fid=103 type=opaque value="opaque value 1"
    field = omnmmsgPayloadIter_next(iterOpaque2, NULL, copy);
    omnmmsgFieldPayload_getName (field, NULL, NULL, &nameactual);
    omnmmsgFieldPayload_getFid (field, NULL, NULL, &fidactual);
    omnmmsgFieldPayload_getType (field, &typeactual);
    omnmmsgFieldPayload_getOpaque (field, (const void**) &buf1, &buf1Len);
    omnmmsgFieldPayload_getAsString (field, mPayloadBase, buf2, sizeof(buf2));
    ASSERT_EQ (MAMA_FIELD_TYPE_OPAQUE, typeactual);
    ASSERT_EQ (fid4, fidactual);
    ASSERT_STREQ (name4, nameactual);
    ASSERT_STREQ (expected4, (const char*) buf1);
    //ASSERT_STREQ (expected4, buf2);

    // no more fields
    field = omnmmsgPayloadIter_next(iterOpaque2, NULL, copy);
    ASSERT_EQ (NULL, field);

    // compare the two as strings
    const char* payloadStr1 = omnmmsgPayload_toString(mPayloadBase);
    const char* payloadStr2 = omnmmsgPayload_toString(copy);
    ASSERT_STREQ (payloadStr2, payloadStr1);

    omnmmsgPayload_destroy (copy);
}

TEST_F(OmnmTests, CreateAndUpdateShrinkingStrings)
{
    mama_i64_t actual1 = 0;
    const char* actual2a = NULL;
    const char* actual2b = NULL;
    mama_i64_t actual3 = 0;
    mama_fid_t fid1 = 101;
    mama_fid_t fid2 = 102;
    mama_fid_t fid3 = 103;
    mama_i64_t expected1 = 12398;
    const char* expected2a = "str_longer";
    const char* expected2b = "str_shrt";
    mama_i64_t expected3 = 12400;

    omnmmsgPayload_addI64 (mPayloadBase, NULL, fid1, expected1);
    omnmmsgPayload_addString (mPayloadBase, NULL, fid2, expected2a);
    omnmmsgPayload_addI64 (mPayloadBase, NULL, fid3, expected3);
    omnmmsgPayload_getString (mPayloadBase, NULL, fid2, &actual2a);
    EXPECT_STREQ (expected2a, actual2a);

    omnmmsgPayload_updateString (mPayloadBase, NULL, fid2, expected2b);

    omnmmsgPayload_getI64 (mPayloadBase, NULL, fid1, &actual1);
    omnmmsgPayload_getString (mPayloadBase, NULL, fid2, &actual2b);
    omnmmsgPayload_getI64 (mPayloadBase, NULL, fid3, &actual3);

    EXPECT_EQ (expected1, actual1);
    EXPECT_STREQ (expected2b, actual2b);
    EXPECT_EQ (expected3, actual3);
}

TEST_F(OmnmTests, CreateAndUpdateGrowingStrings)
{
    mama_fid_t fid = 102;
    const char* actaullong = NULL;
    const char* actaulshort = NULL;
    const char* expectedshort = "str_shrt";
    const char* expectedlong = "str_longer";

    omnmmsgPayload_addString (mPayloadBase, NULL, fid, expectedshort);
    omnmmsgPayload_getString (mPayloadBase, NULL, fid, &actaulshort);
    EXPECT_STREQ (expectedshort, actaulshort);

    omnmmsgPayload_updateString (mPayloadBase, NULL, fid, expectedlong);
    omnmmsgPayload_getString (mPayloadBase, NULL, fid, &actaullong);
    EXPECT_STREQ (expectedlong, actaullong);

    omnmmsgPayload_updateString (mPayloadBase, NULL, fid, expectedshort);
    omnmmsgPayload_getString (mPayloadBase, NULL, fid, &actaulshort);
    EXPECT_STREQ (expectedshort, actaulshort);
}
