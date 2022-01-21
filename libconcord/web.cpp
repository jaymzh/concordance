/*
 * vim:tw=80:ai:tabstop=4:softtabstop=4:shiftwidth=4:expandtab
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (C) Copyright Kevin Timmerman 2007
 * (C) Copyright Phil Dibowitz 2008
 */

#include <stdarg.h>
#include <string.h>
#include <curl/curl.h>
#include "libconcord.h"
#include "lc_internal.h"
#include "hid.h"
#include "remote.h"
#include "xml_headers.h"

static const uint8_t urlencodemap[32]={
    0xFF, 0xFF, 0xFF, 0xFF,  //   0 Control chars
    0x7D,                    //  32 & % $ # " <space>
    0x98,                    //  40 / , +
    0x00,                    //  48
    0xFC,                    //  56 ? > = < ; :
    0x01,                    //  64 @
    0x00,                    //  72
    0x00,                    //  80
    0x78,                    //  88 ^ ] \ [
    0x01,                    //  96 '
    0x00,                    // 104
    0x00,                    // 112
    0x78,                    // 120 ~ } | {
    0xFF, 0xFF, 0xFF, 0xFF,  // 128
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
};

/*
 * This function does C-style string formatting, but uses a C++ string and
 * automatically handles buffer sizing.  It is intended to be used in place of
 * sprintf()/snprintf() where we don't necessarily know the required buffer
 * size in advance.  The formatted string is appended to the supplied C++
 * string.
 */
void format_string(string *str, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int size = vsnprintf(NULL, 0, format, args);
    va_end(args);
    size++; // Add room for \0
    char *buffer = new char[size];
    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    va_end(args);
    *str += buffer;
    delete[] buffer;
}

static void UrlEncode(const char *in, string &out)
{
    out = "";
    const size_t len = strlen(in);
    for(size_t i = 0; i < len; ++i) {
        const char c = in[i];
        if (c == ' ') {
            out += '+';
        } else if (c == '(') {
            out += "%28";
        } else if (c == ')') {
            out += "%29";
        } else if(urlencodemap[c>>3] & (1 << (c & 7))) {
            char hex[4];
            sprintf(hex, "%%%02X", c);
            out += hex;
        } else
            out += c;
    }
}

#ifndef _DEBUG
static size_t dummy_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    return size * nmemb;
}
#endif

static int Zap(string &server, string &path, string &cookie, string &post)
{
    int ret = 0;
    string url = "https://" + server + "/" + path;

    curl_global_init(CURL_GLOBAL_ALL);

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);
        curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
#ifdef _DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, stderr);
#else
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dummy_cb);
#endif

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            debug("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            ret = LC_ERROR_OS_NET;
        }

        curl_easy_cleanup(curl);
    } else {
        debug("curl_easy_init() failed");
        ret = LC_ERROR_OS_NET;
    }

    curl_global_cleanup();

    return ret;
}

// find is the tag.
// data is the data to search through
// data_size is the size of data
// found is where we found it
// s, if passed, will hold the strong of the entire element, unless
// If find_attributes is set to true, and s is not null, will fill s with the
// attributes for the requested tag. For example, if you have:
// <ELEMENT A="B" C="D"/> and you search for a tag called "ELEMENT" the
// function will return A="B" C="D".
int GetTag(const char *find, uint8_t* data, uint32_t data_size, uint8_t *&found,
           string *s=NULL, bool find_attributes=false)
{
    char tag_name_end = '>';
    char tag_start = '<';
    char tag_end = '>';
    if (find_attributes) {
        tag_name_end = ' ';
        tag_start = '/';
        tag_end = '/';
    }

    const size_t find_len = strlen(find);
    uint8_t * search = data;

    // Consume tags until there aren't any left
    while (1) {
        // Loop searching for start of tag character
        while (1) {
            if (*search == '<') {
                break;
            }
            if (search >= data + data_size) {
                return -1;
            }
            search++;
        }
        // Validate there's enough string left to hold the tag name
        uint32_t needed_len = find_len + 2;
        uint32_t left_len = data_size - (search - data);
        if (left_len < needed_len) {
            return -1;
        }
        // Point past <, at tag name
        search++;
        // Check to see if this is the tag we want
        if (search[find_len] == tag_name_end &&
            !strnicmp(find, reinterpret_cast<const char*>(search), find_len)) {
            // Point past >, at tag content
            search += find_len + 1;

            found = search;

            /*
              * If a string pointer was passed in, then add the
              * contents of this entire tag to the string
              */
            if (s) {
                *s = "";
                /*
                 * Here we keep adding chars until the next tag
                 * which, in theory, should be the end-tag.
                 */
                while (*search && *search != tag_start) {
                    *s += *search;
                    search++;
                    if (search >= data + data_size) {
                        break;
                    }
                }
            }
            return 0;
        }

        // Loop searching for end of tag character
        while (1) {
            if (*search == tag_end) {
                break;
            }
            if (search >= data + data_size) {
                return -1;
            }
            search++;
        }
    }
}

// Given a set of XML attributes, e.g., ATTR1="VALUE1" ATTR2="VALUE2", this
// function will find a given attribute and return its value.
int GetAttribute(const char *find, string data, string *result)
{
    if ((find == NULL) || (result == NULL))
        return -1;

    size_t start_pos;
    size_t end_pos;
    string to_find = find;
    to_find.append("=\"");
    
    start_pos = data.find(to_find);
    if (start_pos == string::npos)
        return -1;
    start_pos += to_find.length();

    end_pos = data.find("\"", start_pos);
    if (end_pos == string::npos)
        return -1;

    *result = data.substr(start_pos, end_pos - start_pos);
    return 0;
}

int encode_ir_signal(uint32_t carrier_clock, uint32_t *ir_signal,
                     uint32_t ir_signal_length, string *learn_seq)
{    /*
     * Encode ir_signal into string accepted by Logitech server
     */
    char s[32];
    
    if ((learn_seq == NULL) || (ir_signal == NULL) || (ir_signal_length == 0)) {
        return LC_ERROR;
    }
    if (carrier_clock > 0xFFFF) {
        sprintf(s, "F%08X", carrier_clock);
    } else {
        sprintf(s, "F%04X", carrier_clock);
    }
    *learn_seq = s;
    for (unsigned int n = 0; n < ir_signal_length; ) {
        if (ir_signal[n] > 0xFFFF) {
            sprintf(s, "P%08X", ir_signal[n++]);
        } else {
            sprintf(s, "P%04X", ir_signal[n++]);
        }
        *learn_seq += s;
        if (ir_signal[n] > 0xFFFF) {
            sprintf(s, "S%08X", ir_signal[n++]);
        } else {
            sprintf(s, "S%04X", ir_signal[n++]);
        }
        *learn_seq += s;
    }
    return 0;
}

void add_usbnet_headers(char *post_data, TRemoteInfo &ri)
{
    sprintf(post_data+strlen(post_data), post_xml_usbnet1, ri.home_id, 
            ri.node_id, ri.tid);
    for (int i=0; i<ri.num_regions; i++) {
        sprintf(post_data+strlen(post_data), post_xml_usbnet_region,
                ri.region_ids[i], ri.region_versions[i]);
    }
    sprintf(post_data+strlen(post_data), "%s", post_xml_usbnet2);
    sprintf(post_data+strlen(post_data), "%s", ri.xml_user_rf_setting);
    sprintf(post_data+strlen(post_data), "%s", post_xml_usbnet3);
}

int Post(uint8_t *xml, uint32_t xml_size, const char *root, TRemoteInfo &ri,
         bool has_userid, bool add_cookiekeyval, bool z_post, string *learn_seq,
         string *learn_key)
{
    uint8_t *x = xml;
    int err;
    if ((err = GetTag(root, x, xml_size - (x - xml), x)))
        return err;

    string server, path, cookie, userid;

    if ((err = GetTag("SERVER", x, xml_size - (x - xml), x, &server)))
        return err;
    if ((err = GetTag("PATH", x, xml_size - (x - xml), x, &path)))
        return err;
    if ((err = GetTag("VALUE", x, xml_size - (x - xml), x, &cookie)))
        return err;
    if (has_userid) {
        uint8_t *n = 0;
        if ((err = GetTag("VALUE", x, xml_size - (x - xml), n, &userid)))
            return err;
    }

    /*
     * For some architectures the website leaves one required value out of
     * the cookie. Who knows why, but we allow the user to tell us to add
     * it.
     */
    if (add_cookiekeyval) {
        cookie += ";CookieKeyValue=";
        cookie += ri.serial1;
        cookie += ri.serial2;
        cookie += ri.serial3;
    }

    debug("Connecting to: %s", server.c_str());
    debug("Path: %s", path.c_str());
    debug("Cookie: %s", cookie.c_str());
    debug("UserId: %s", userid.c_str());

    string post;
    if (learn_seq == NULL) {
        string serial;
        format_string(&serial, "%s%s%s", ri.serial1, ri.serial2, ri.serial3);
        string post_data;
        if (z_post) {
            format_string(&post_data, z_post_xml, ri.hw_ver_major,
                    ri.hw_ver_minor, ri.flash_mfg, ri.flash_id, ri.fw_ver_major,
                    ri.fw_ver_minor);
        } else {
            format_string(&post_data, post_xml, ri.fw_ver_major,
                    ri.fw_ver_minor, ri.fw_type, serial.c_str(),
                    ri.hw_ver_major, ri.hw_ver_minor, ri.hw_ver_micro,
                    ri.flash_mfg, ri.flash_id, ri.protocol, ri.architecture,
                    ri.skin);
            format_string(&post_data, "%s", post_xml_trailer);
        }

        debug("post data: %s", post_data.c_str());

        string post_data_encoded;
        UrlEncode(post_data.c_str(), post_data_encoded);

        post = "Data=" + post_data_encoded;
    } else {
        post = "IrSequence=" + *learn_seq + "&KeyName=" + *learn_key;
    }

    if (has_userid) {
        post += "&UserId=" + userid;
    }

    debug("%s", post.c_str());

    return Zap(server, path, cookie, post);
}
