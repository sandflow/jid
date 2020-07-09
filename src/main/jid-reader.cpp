/*
 * Copyright (c), Pierre-Anthony Lemieux (pal@palemieux.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <KM_fileio.h>
#include <KM_prng.h>
#include <AS_02.h>
#include <Metadata.h>
#include <assert.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include <iostream>
#include <string>
#include <map>
#include <iomanip>
#include <fstream>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <stdio.h>
#endif



/* enumeration of supported input formats */


enum class OutputFormats {
    J2C,
    MJC
};

std::istream& operator>>(std::istream& is, OutputFormats& f) {

    std::string s;

    is >> s;

    if (s == "J2C") {
        f = OutputFormats::J2C;
    } else if (s == "MJC") {
        f = OutputFormats::MJC;
    } else {
        throw std::runtime_error("Unknown input file format");
    }

    return is;
}


std::ostream& operator<<(std::ostream& os, const OutputFormats& f) {

    switch (f) {
    case OutputFormats::J2C:
        os << "J2C";
        break;
    case OutputFormats::MJC:
        os << "MJC";
        break;
    }

    return os;
}

int main(int argc, const char* argv[]) {

    ASDCP::Result_t result = ASDCP::RESULT_OK;

    /* initialize command line options */

    boost::program_options::options_description cli_opts{ "Unwraps JPEG 2000 codestreams from IMF Image Track Files" };

    cli_opts.add_options()
        ("help", "Prints usage")
        ("format", boost::program_options::value<OutputFormats>()->default_value(OutputFormats::J2C), "Output format\n"
            "  MJC: \t16-byte header followed by a sequence of J2C codestreams, each preceded by a 4-byte little-endian length\n"
            "  J2C: \tindividual JPEG 2000 codestreams")
        ("buffer-size", boost::program_options::value<uint32_t>()->default_value(8192*8192*3*2 /* 8K */), "Read buffer size (8K 4:4:4 16-bit if unspecified)")
        ("out", boost::program_options::value<std::string>(), "Output path (or stdout if none is specified)")
        ("in", boost::program_options::value<std::string>()->required(), "Input MXF file path");

    boost::program_options::variables_map cli_args;

    try {

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cli_opts), cli_args);

        boost::program_options::notify(cli_args);

        /* display help options */

        if (cli_args.count("help")) {
            std::cout << cli_opts << "\n";
            return 1;
        }

        /* setup the output */

        const OutputFormats format = cli_args["format"].as<OutputFormats>();

        FILE* mjc_output = NULL;

        switch (format) {

        case OutputFormats::J2C:

            if (cli_args["out"].empty() || (! Kumu::PathIsDirectory(cli_args["out"].as<std::string>()) )) {
                throw std::runtime_error("Output path must be an existing directory when J2C output format is selected.");
            }

            break;

        case OutputFormats::MJC:

            if (cli_args["out"].empty()) {

                /* open stdin in binary mode */

#ifdef WIN32

                int mode = _setmode(_fileno(stdin), O_BINARY);

                if (mode == -1) {
                    throw std::runtime_error("Cannot reopen stdout");
                }

                mjc_output = stdin;

#else

                mjc_output = freopen(NULL, "rb", stdin);

                if (!mjc_output) {
                    throw std::runtime_error("Cannot reopen stdout");
                }

#endif

            } else {
            
                /* create output file */
            
                mjc_output = fopen(cli_args["out"].as<std::string>().c_str(), "wb");

                if (!mjc_output) {
                    throw std::runtime_error("Cannot create output file");
                }

            }



            break;

        default:

            throw std::runtime_error("Unknown output format");

        }

        /* open input file */

        AS_02::JP2K::MXFReader reader;

        result = reader.OpenRead(cli_args["in"].as<std::string>());

        if (result.Failure()) {
            throw std::runtime_error("Cannot open input file");
        }

        if (format == OutputFormats::MJC) {

            /* output MJC header */

            ASDCP::MXF::InterchangeObject* obj = 0;

            result = reader.OP1aHeader().GetMDObjectByType(
                reader.OP1aHeader().m_Dict->Type(ASDCP::MDD_RGBAEssenceDescriptor).ul,
                &obj
            );

            uint32_t flags = result.Success() ? 2 /* KDU_SIMPLE_VIDEO_RGB */ : 1 /* KDU_SIMPLE_VIDEO_YCC */;

            ASDCP::Rational edit_rate;

            if (!ASDCP::MXF::GetEditRateFromFP(reader.OP1aHeader(), edit_rate)) {

                throw std::runtime_error("Cannot read edit rate from input file");

            }

            std::array<uint8_t, 16> header = {
                'M',
                'J',
                'C',
                '2',
                (uint8_t) ((edit_rate.Numerator >> 24) & 0xFF),
                (uint8_t) ((edit_rate.Numerator >> 16) & 0xFF),
                (uint8_t) ((edit_rate.Numerator >> 8) & 0xFF),
                (uint8_t) (edit_rate.Numerator & 0xFF),
                (uint8_t) ((edit_rate.Denominator >> 24) & 0xFF),
                (uint8_t) ((edit_rate.Denominator >> 16) & 0xFF),
                (uint8_t) ((edit_rate.Denominator >> 8) & 0xFF),
                (uint8_t) (edit_rate.Denominator & 0xFF),
                (uint8_t) ((flags >> 24) & 0xFF),
                (uint8_t) ((flags >> 16) & 0xFF),
                (uint8_t) ((flags >> 8) & 0xFF),
                (uint8_t) (flags & 0xFF),
            };

            fwrite(header.data(), sizeof(header), 1, mjc_output);

        }

        /* Codestream frame buffer for the MXF writer */

        ASDCP::JP2K::FrameBuffer fb(cli_args["buffer-size"].as<uint32_t>());

        uint32_t frame_count = reader.AS02IndexReader().GetDuration();

        for (uint32_t i = 0; i < frame_count; i++) {

            result = reader.ReadFrame(i, fb);

            if (result.Failure()) {
                throw std::runtime_error("Cannot read frame");
            }

            if (format == OutputFormats::J2C) {

                /* write single J2C */

                std::stringstream ss;

                ss << cli_args["out"].as<std::string>() << "/" << std::setfill('0') << std::setw(6) << i << ".j2c";

                std::ofstream f(ss.str(), std::ios_base::out | std::ios_base::binary);
                   
                if (!f.good()) {
                    throw std::runtime_error("Cannot open output file");
                }

                f.write((const char*) fb.RoData(), fb.Size());

                f.close();
            
            } else {

                /* write single MJC frame */

                uint32_t csz = KM_i32_BE(fb.Size());

                fwrite(&csz, 4, 1, mjc_output);

                fwrite((const char*)fb.RoData(), 1, fb.Size(), mjc_output);
                        
            }

        }

        /* close reader */

        result = reader.Close();

        if (ASDCP_FAILURE(result)) {
            throw std::runtime_error("Cannot close file");
        }

    } catch (boost::program_options::required_option e) {

        std::cout << cli_opts << std::endl;
        return 1;

    } catch (std::runtime_error e) {

        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
