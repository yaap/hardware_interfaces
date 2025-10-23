/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <getopt.h>
#include <unistd.h>

#include <charconv>
#include <cstring>
#include <iostream>

void printUsage() {
    std::cout << "Usage: run-test-inference --job-priority <priority> --original-uid <uid>"
              << std::endl;
}

int main(int argc, char** argv) {
    int jobPriority = 500;
    int originalUid = -1;

    const struct option longOptions[] = {{"job-priority", required_argument, nullptr, 'p'},
                                         {"original-uid", required_argument, nullptr, 'u'},
                                         {nullptr, 0, nullptr, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "p:u:", longOptions, nullptr)) != -1) {
        switch (opt) {
            case 'p': {
                auto result = std::from_chars(optarg, optarg + std::strlen(optarg), jobPriority);
                if (result.ec != std::errc()) {
                    std::cerr << "ERROR: invalid job priority " << optarg << std::endl;
                    printUsage();
                    return 1;
                }
                break;
            }
            case 'u': {
                auto result = std::from_chars(optarg, optarg + std::strlen(optarg), originalUid);
                if (result.ec != std::errc()) {
                    std::cerr << "ERROR: invalid UID " << optarg << std::endl;
                    printUsage();
                    return 1;
                }
                break;
            }
            default:
                printUsage();
                return 1;
        }
    }

    std::cout << "Job Priority: " << jobPriority << std::endl;
    if (originalUid >= 0) {
        std::cout << "Original UID: " << originalUid << std::endl;
    }

    // TODO: Perform inference here
    std::cerr << "ERROR: not implemented" << std::endl;

    return 1;
}
