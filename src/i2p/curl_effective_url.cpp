#include "curl_effective_url.h"
#include <iostream>
#include <curl/curl.h>

/*int main(int argc, char *argv[]) {*/
/*    CURL *curl;*/
/*    CURLcode res;*/
/**/
/*    curl = curl_easy_init();*/
/*    if (curl) {*/
/*        const char *url = argv[1]; // Replace with the actual URL*/
/*        curl_easy_setopt(curl, CURLOPT_URL, url);*/
/**/
/*        // Follow redirects*/
/*        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);*/
/**/
/*        // Suppress output (similar to -o /dev/null)*/
/*        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);*/
/*        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](char *, size_t size, size_t nmemb, void *) {*/
/*            return size * nmemb; // Ignore downloaded data*/
/*        });*/
/**/
/*        // Perform the request*/
/*        res = curl_easy_perform(curl);*/
/*        if (res == CURLE_OK) {*/
/*            // Get the effective URL*/
/*            char *effective_url = nullptr;*/
/*            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);*/
/**/
/*            if (effective_url) {*/
/*                std::cout << "Effective URL: " << effective_url << std::endl;*/
/*            } else {*/
/*                std::cerr << "Could not retrieve effective URL." << std::endl;*/
/*            }*/
/*        } else {*/
/*            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;*/
/*        }*/
/**/
/*        // Cleanup*/
/*        curl_easy_cleanup(curl);*/
/*    } else {*/
/*        std::cerr << "Could not initialize cURL." << std::endl;*/
/*    }*/
/**/
/*    return 0;*/
/*}*/
std::string get_url_redirect(std::string url) {
    CURL *curl;
    CURLcode res;
    char *effective_url = nullptr;
    std::string str = "";
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Suppress output (similar to -o /dev/null)
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](char *, size_t size, size_t nmemb, void *) {
            return size * nmemb; // Ignore downloaded data
        });

        // Perform the request
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            // Get the effective URL
            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);

            if (effective_url) {
                std::cout << "Effective URL: " << effective_url << std::endl;
                str = std::string(effective_url);
            } else {
                std::cerr << "Could not retrieve effective URL." << std::endl;
            }
        } else {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        }

        // Cleanup
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Could not initialize cURL." << std::endl;
    }
    /*std::cout << "sadf: " << (effective_url == nullptr) << std::endl;*/
    return str;
}

