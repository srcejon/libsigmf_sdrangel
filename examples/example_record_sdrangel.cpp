/*
 *    Copyright 2019 DeepSig Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <iostream>
#include <nlohmann/json.hpp>

#include "sigmf_core_generated.h"
#include "sigmf_multirecordings_generated.h"
#include "sigmf_sdrangel_generated.h"
#include "libsigmf/sigmf.h"

int main() {

    /*
     * Create a record with 2 namespace available in it. Note that antenna namespace doesnt have capture segments.
     * Also use this to show off using the sigmf::Global, sigmf::Capture, and sigmf::Annotation classes. These
     * classes are light wrappers around the sigmf::VariadicDataClass but make it easier to express intent of
     * what kind of stuff (in sigmf-lingo) the underlying class is supposed to hold.
     */
    sigmf::SigMF<sigmf::Global<core::DescrT, multirecordings::DescrT>,
            sigmf::Capture<core::DescrT, multirecordings::DescrT, sdrangel::DescrT>,
            sigmf::Annotation<core::DescrT> > latest_record;
    latest_record.global.access<core::GlobalT>().author = "f4exb";
    latest_record.global.access<core::GlobalT>().description = "Example of multirecordings";
    latest_record.global.access<core::GlobalT>().sample_rate = 96000.0;
    latest_record.global.access<core::GlobalT>().hw = "RTLSDR";
    latest_record.global.access<core::GlobalT>().recorder = "SDRangel";
    latest_record.global.access<core::GlobalT>().version = "0.0.2";
    latest_record.global.access<multirecordings::GlobalT>().master = "example-channel-0";

    //
    std::vector<std::string> multirecordingFiles;
    multirecordingFiles.push_back("example-channel-0");
    multirecordingFiles.push_back("example-channel-1");

    // Add a capture segment
    auto recording_capture = sigmf::Capture<core::DescrT, multirecordings::DescrT, sdrangel::DescrT>();
    recording_capture.get<core::DescrT>().frequency = 433.15e6;
    recording_capture.get<core::DescrT>().sample_start = 0;
    recording_capture.get<core::DescrT>().length = 90000;
    recording_capture.get<multirecordings::DescrT>().streams = multirecordingFiles;
    latest_record.captures.emplace_back(recording_capture);

    // Add a second capture segment with change of sample rate (sdrangel)
    recording_capture.get<core::DescrT>().frequency = 433.95e6;
    recording_capture.get<core::DescrT>().sample_start = 90000;
    recording_capture.get<core::DescrT>().length = 50000;
    recording_capture.get<multirecordings::DescrT>().streams.clear(); // do not repeat
    recording_capture.get<sdrangel::DescrT>().sample_rate = 100000;
    latest_record.captures.emplace_back(recording_capture);

    // Add a third capture segment missing streams
    recording_capture.get<core::DescrT>().frequency = 433.85e6;
    recording_capture.get<core::DescrT>().sample_start = 140000;
    recording_capture.get<core::DescrT>().length = 60000;
    recording_capture.get<sdrangel::DescrT>().sample_rate = 0; // do not repeat
    latest_record.captures.emplace_back(recording_capture);

    auto expected_json = R"({
  "annotations": [],
  "captures": [
    {
      "core:frequency": 433150000.0,
      "core:length": 90000,
      "multirecordings:streams": [
        "example-channel-0",
        "example-channel-1"
      ]
    },
    {
      "core:frequency": 433950000.0,
      "core:length": 50000,
      "core:sample_start": 90000,
      "sdrangel:sample_rate": 100000
    },
    {
      "core:frequency": 433850000.0,
      "core:length": 60000,
      "core:sample_start": 140000
    }
  ],
  "global": {
    "core:author": "f4exb",
    "core:description": "Example of multirecordings",
    "core:hw": "RTLSDR",
    "core:recorder": "SDRangel",
    "core:sample_rate": 96000.0,
    "core:version": "0.0.2",
    "multirecordings:master": "example-channel-0"
  }
})";

    std::string json_record = json(latest_record).dump(2);

    std::cout << json_record << std::endl;
    //assert(expected_json == json_record);
    //std::cout << "example_record_with_multiple_namespaces passed" << std::endl;

    auto as_json = json::parse(json_record);

    std::cout << "*** Using multirecordings and sdrangel extensions" << std::endl;

    sigmf::SigMF<sigmf::Global<core::DescrT, multirecordings::DescrT>,
            sigmf::Capture<core::DescrT, multirecordings::DescrT, sdrangel::DescrT>,
            sigmf::Annotation<core::DescrT> > read_record = as_json;

    std::cout << "Author...: " << read_record.global.access<core::GlobalT>().author << std::endl;
    std::cout << "Recorder : " << read_record.global.access<core::GlobalT>().recorder << std::endl;
    std::cout << "SRate....: " << read_record.global.access<core::GlobalT>().sample_rate << std::endl;
    std::cout << "Master...: " << read_record.global.access<multirecordings::GlobalT>().master << std::endl;

    for (auto capture : read_record.captures)
    {
        std::cout << "----- capture -----" << std::endl;
        std::cout << "Start....: " << capture.access<core::CaptureT>().sample_start << std::endl;
        std::cout << "Length...: " << capture.access<core::CaptureT>().length << std::endl;
        std::cout << "Frequency: " << capture.access<core::CaptureT>().frequency << std::endl;
        std::cout << "SRate....: " << capture.access<sdrangel::CaptureT>().sample_rate << std::endl;

        for (auto stream : capture.access<multirecordings::CaptureT>().streams) {
            std::cout << "Stream...: " << stream << std::endl;
        }
    }

    std::cout << std::endl << "*** Using no extensions" << std::endl;

    sigmf::SigMF<sigmf::Global<core::DescrT>,
            sigmf::Capture<core::DescrT>,
            sigmf::Annotation<core::DescrT> > read_record_std = as_json;

    std::cout << "Author...: " << read_record.global.access<core::GlobalT>().author << std::endl;
    std::cout << "Recorder : " << read_record.global.access<core::GlobalT>().recorder << std::endl;
    std::cout << "SRate....: " << read_record.global.access<core::GlobalT>().sample_rate << std::endl;

    for (auto capture : read_record.captures)
    {
        std::cout << "----- capture -----" << std::endl;
        std::cout << "Start....: " << capture.access<core::CaptureT>().sample_start << std::endl;
        std::cout << "Length...: " << capture.access<core::CaptureT>().length << std::endl;
        std::cout << "Frequency: " << capture.access<core::CaptureT>().frequency << std::endl;
    }

    return 0; // assert passed, we're good :+1:
}