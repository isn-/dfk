/**
 * @copyright
 * Copyright (c) 2016 Stanislav Ivochkin
 * Licensed under the MIT License:
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <dfk/urlencoding.h>
#include <dfk/internal.h>
#include <ut.h>


static void ut_urldecode(const char* encoded, size_t encodedlen, const char* expected, size_t expectedlen)
{
  char* decoded = malloc(encodedlen);
  size_t bytesdecoded;
  size_t bytesread = dfk_urldecode((char*) encoded, encodedlen, decoded, &bytesdecoded);
  EXPECT(bytesread == encodedlen);
  EXPECT(bytesdecoded == expectedlen);
  EXPECT(!strncmp(decoded, expected, expectedlen));
  free(decoded);
}


#define UT_URLDECODE(encoded, expected) \
  ut_urldecode((encoded), DFK_SIZE(encoded), (expected), DFK_SIZE(expected));


TEST(urlencoding, decode_empty)
{
  UT_URLDECODE("", "");
}


TEST(urlencoding, decode_simple)
{
  UT_URLDECODE("%68%65%6C%6C%6F%2C%20%77%6F%72%6C%64", "hello, world");
}

TEST(urlencoding, decode_simple_mix)
{
  UT_URLDECODE("%68ell%6F%2C%20%77%6F%72ld", "hello, world");
}

TEST(urlencoding, decode_simple_lower)
{
  UT_URLDECODE("%68%65%6c%6c%6f%2c%20%77%6f%72%6c%64", "hello, world");
}

TEST(urlencoding, decode_coverage)
{
  UT_URLDECODE(
    "%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F"
    "%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F"
    "%20%21%22%23%24%25%26%27%28%29%2A%2B%2C%2D%2E%2F"
    "%30%31%32%33%34%35%36%37%38%39%3A%3B%3C%3D%3E%3F"
    "%40%41%42%43%44%45%46%47%48%49%4A%4B%4C%4D%4E%4F"
    "%50%51%52%53%54%55%56%57%58%59%5A%5B%5C%5D%5E%5F"
    "%60%61%62%63%64%65%66%67%68%69%6A%6B%6C%6D%6E%6F"
    "%70%71%72%73%74%75%76%77%78%79%7A%7B%7C%7D%7E%7F"
    "%80%81%82%83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F"
    "%90%91%92%93%94%95%96%97%98%99%9A%9B%9C%9D%9E%9F"
    "%A0%A1%A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF"
    "%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%BA%BB%BC%BD%BE%BF"
    "%C0%C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF"
    "%D0%D1%D2%D3%D4%D5%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF"
    "%E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF"
    "%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FB%FC%FD%FE%FF",
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
    "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
    "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
    "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f"
    "\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"
    "\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
    "\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f"
    "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
    "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
    "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
    "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
    "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
    "\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
    "\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
    "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"
  );
}

TEST(urlencoding, decode_bad_input)
{
  /* %2C replaced with %2G */
  char encoded[] = "%68%65%6C%6C%6F%2G%20%77%6F%72%6C%64";
  char decoded[DFK_SIZE(encoded)] = {0};
  size_t byteswritten;
  size_t bytesread = dfk_urldecode(encoded, DFK_SIZE(encoded), decoded, &byteswritten);
  EXPECT(bytesread == 15);
  EXPECT(byteswritten == 5);
  EXPECT(!strncmp(decoded, "hello", byteswritten));
}

