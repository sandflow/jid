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
#include "CodestreamSequence.h"
#include "J2KProfileULMap.h"

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <stdio.h>
#endif

/* authoring identification info written to file headers */

class DCDM2IMFWriterInfo : public ASDCP::WriterInfo {
public:
    DCDM2IMFWriterInfo() {
        static byte_t default_ProductUUID_Data[ASDCP::UUIDlen] =
        { 0x92, 0x7f, 0xc4, 0xd1, 0x89, 0xa3, 0x4f, 0x88, 0x88, 0xbb, 0xd3, 0x63, 0xed, 0x33, 0x08, 0x4a };

        memcpy(ProductUUID, default_ProductUUID_Data, ASDCP::UUIDlen);
        CompanyName = "Sandflow Consulting LLC";
        ProductName = "dcdm2imf";
        ProductVersion = "1.0-beta1";
    }
};


/* hard-coded UL definitions */

std::array<uint8_t, 16> CodingEquations_ITU601 = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x02, 0x01, 0x00, 0x00 };
std::array<uint8_t, 16> CodingEquations_ITU709 = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x02, 0x02, 0x00, 0x00 };
std::array<uint8_t, 16> CodingEquations_ITU2020_NCL = { 0x06, 0x0e, 0x2b, 0x34, 04, 0x01, 0x01, 0x0d, 0x04, 0x01, 0x01, 0x01, 0x02, 0x06, 0x00, 0x00 };

std::array<uint8_t, 16> TransferCharacteristic_ITU709 = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00 };
std::array<uint8_t, 16> TransferCharacteristic_IEC6196624_xvYCC = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x04, 0x01, 0x01, 0x01, 0x01, 0x08, 0x00, 0x00 };
std::array<uint8_t, 16> TransferCharacteristic_ITU2020 = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0e, 0x04, 0x01, 0x01, 0x01, 0x01, 0x09, 0x00, 0x00 };
std::array<uint8_t, 16> TransferCharacteristic_SMPTEST2084 = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x04, 0x01, 0x01, 0x01, 0x01, 0x0a, 0x00, 0x00 };
std::array<uint8_t, 16> TransferCharacteristic_CinemaMezzanine_DCDM = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0e, 0x04, 0x01, 0x01, 0x01, 0x01, 0x13, 0x00, 0x00 };

std::array<uint8_t, 16> ColorPrimaries_SMPTE170M = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x06, 0x04, 0x01, 0x01, 0x01, 0x03, 0x01, 0x00, 0x00 };
std::array<uint8_t, 16> ColorPrimaries_ITU470_PAL = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x06, 0x04, 0x01, 0x01, 0x01, 0x03, 0x02, 0x00, 0x00 };
std::array<uint8_t, 16> ColorPrimaries_ITU709 = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x06, 0x04, 0x01, 0x01, 0x01, 0x03, 0x03, 0x00, 0x00 };
std::array<uint8_t, 16> ColorPrimaries_ITU2020 = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x04, 0x01, 0x01, 0x01, 0x03, 0x04, 0x00, 0x00 };
std::array<uint8_t, 16> ColorPrimaries_P3D65 = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x04, 0x01, 0x01, 0x01, 0x03, 0x06, 0x00, 0x00 };
std::array<uint8_t, 16> ColorPrimaries_CinemaMezzanine = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x04, 0x01, 0x01, 0x01, 0x03, 0x08, 0x00, 0x00 };

std::array<uint8_t, 16> HTJ2KPictureCodingSchemeGeneric = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0D, 0x04, 0x01, 0x02, 0x02, 0x03, 0x01, 0x08, 0x01 };

namespace ASDCP {

    /* TODO: this is necessary since the symbol is never exported by the core libraries */

    Result_t JP2K_PDesc_to_MD(const ASDCP::JP2K::PictureDescriptor& PDesc,
        const ASDCP::Dictionary& dict,
        ASDCP::MXF::GenericPictureEssenceDescriptor& GenericPictureEssenceDescriptor,
        ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor);

    /* TODO: this is necessary since the symbol is never exported by the core libraries */

    Result_t JP2K::ParseMetadataIntoDesc(const FrameBuffer& FB, PictureDescriptor& PDesc, byte_t* start_of_data);


    /* overloads needed for boost::program_options */

    std::istream& operator>>(std::istream& is, ASDCP::Rational& r) {

        std::string s;

        is >> s;

        std::vector<std::string> params;

        boost::split(params, s, boost::is_any_of("/"));

        if (params.size() == 0 || params.size() > 2) {
            throw std::runtime_error("Cannot read rational");
        }

        r = ASDCP::Rational(
            std::stoi(params[0]),
            params.size() == 1 ? 1 : std::stoi(params[1])
        );

        return is;
    }

    /* overloads needed for boost::program_options */

    std::ostream& operator<<(std::ostream& os, const ASDCP::Rational& r) {

        os << r.Numerator << "/" << r.Denominator;

        return os;
    }

}

namespace Kumu {
    /* overloads needed for boost::program_options */

    std::istream& operator>>(std::istream& is, Kumu::UUID& uuid) {

        std::string s;

        is >> s;

        if (!uuid.DecodeHex(s.c_str())) {
            throw std::invalid_argument("Bad UUID");
        }

        return is;
    }


    /* overloads needed for boost::program_options */

    std::ostream& operator<<(std::ostream& os, const Kumu::UUID& uuid) {

        char buf[128];

        if (uuid.EncodeString(buf, sizeof buf)) {

            os << buf;

        } else {

            throw std::invalid_argument("Bad UUID");

        }

        return os;
    }

}

/* enumeration of supported input formats */


enum class InputFormats {
    J2C,
    MJC
};

std::istream& operator>>(std::istream& is, InputFormats& f) {

    std::string s;

    is >> s;

    if (s == "J2C") {
        f = InputFormats::J2C;
    } else if (s == "MJC") {
        f = InputFormats::MJC;
    } else {
        throw std::runtime_error("Unknown input file format");
    }

    return is;
}


std::ostream& operator<<(std::ostream& os, const InputFormats& f) {

    switch (f) {
    case InputFormats::J2C:
        os << "J2C";
        break;
    case InputFormats::MJC:
        os << "MJC";
        break;
    }

    return os;
}

/* quantization */


enum class Quantization {
    QE_2,
    QE_1
};

std::istream& operator>>(std::istream& is, Quantization& q) {

    std::string s;

    is >> s;

    if (s == "QE.1") {
        q = Quantization::QE_1;
    } else if (s == "QE.2") {
        q = Quantization::QE_2;
    } else {
        throw std::runtime_error("Unknown quantization");
    }

    return is;
}


std::ostream& operator<<(std::ostream& os, const Quantization& q) {

    switch (q) {
    case Quantization::QE_2:
        os << "QE.2";
        break;
    case Quantization::QE_1:
        os << "QE.1";
        break;
    }

    return os;
}

/* enumeration of components */

enum class ImageComponents {
    RGB,
    YCbCr,
    XYZ
};

std::istream& operator>>(std::istream& is, ImageComponents& f) {

    std::string s;

    is >> s;

    if (s == "RGB") {
        f = ImageComponents::RGB;
    } else if (s == "YCbCr") {
        f = ImageComponents::YCbCr;
    } else if (s == "XYZ") {
        f = ImageComponents::XYZ;
    } else {
        throw std::runtime_error("Unknown image components");
    }

    return is;
}


std::ostream& operator<<(std::ostream& os, const ImageComponents& f) {

    switch (f) {
    case ImageComponents::RGB:
        os << "RGB";
        break;
    case ImageComponents::YCbCr:
        os << "YCbCr";
        break;
    case ImageComponents::XYZ:
        os << "XYZ";
        break;
    }

    return os;
}

/* enumeration of supported colorimetry */

class EnumeratedColorimetry {
public:

    /* defined colorimetry values */

    static const EnumeratedColorimetry COLOR_1;
    static const EnumeratedColorimetry COLOR_2;
    static const EnumeratedColorimetry COLOR_3;
    static const EnumeratedColorimetry COLOR_4;
    static const EnumeratedColorimetry COLOR_5;
    static const EnumeratedColorimetry COLOR_6;
    static const EnumeratedColorimetry COLOR_7;
    static const EnumeratedColorimetry COLOR_APP4_2;

    static const EnumeratedColorimetry& fromString(const std::string s) {

        auto i = EnumeratedColorimetry::colors_.find(s);

        if (i == EnumeratedColorimetry::colors_.cend()) {
            throw std::runtime_error("Unknown colorimetry");
        }

        return *(i->second);
    }

    static std::string usage() {
        std::stringstream ss;

        ss << "Colorspace:" << std::endl;

        for (auto pair : EnumeratedColorimetry::colors_) {
            ss << pair.first << std::endl;
        }

        return ss.str();
    }

    const std::string& symbol() const {
        return this->symbol_;
    }

    const std::array<uint8_t, 16>& transfer_characteristic() const {
        return this->transfer_characteristic_;
    }

    const std::array<uint8_t, 16>& color_primaries() const {
        return this->color_primaries_;
    }

    const std::array<uint8_t, 16>& coding_equations() const {
        return this->coding_equations_;
    }


private:

    EnumeratedColorimetry(const std::string& symbol,
        const std::array<uint8_t, 16> &transfer_characteristic,
        const std::array<uint8_t, 16> &color_primaries,
        const std::array<uint8_t, 16> &coding_equations) :
        symbol_(symbol),
        transfer_characteristic_(transfer_characteristic),
        color_primaries_(color_primaries),
        coding_equations_(coding_equations)
    {
        if (!this->colors_.insert(std::make_pair(this->symbol_, this)).second) {
            throw std::runtime_error("Existing colorimetry");
        }
    }

    static std::map<std::string, EnumeratedColorimetry*> colors_;

    std::string symbol_;

    std::array<uint8_t, 16> transfer_characteristic_;
    std::array<uint8_t, 16> color_primaries_;
    std::array<uint8_t, 16> coding_equations_;
};

std::map<std::string, EnumeratedColorimetry*> EnumeratedColorimetry::colors_;
const EnumeratedColorimetry EnumeratedColorimetry::COLOR_1("COLOR.1", TransferCharacteristic_ITU709, ColorPrimaries_ITU470_PAL, CodingEquations_ITU601);
const EnumeratedColorimetry EnumeratedColorimetry::COLOR_2("COLOR.2", TransferCharacteristic_ITU709, ColorPrimaries_SMPTE170M, CodingEquations_ITU601);
const EnumeratedColorimetry EnumeratedColorimetry::COLOR_3("COLOR.3", TransferCharacteristic_ITU709, ColorPrimaries_ITU709, CodingEquations_ITU709);
const EnumeratedColorimetry EnumeratedColorimetry::COLOR_4("COLOR.4", TransferCharacteristic_IEC6196624_xvYCC, ColorPrimaries_SMPTE170M, CodingEquations_ITU601);
const EnumeratedColorimetry EnumeratedColorimetry::COLOR_5("COLOR.5", TransferCharacteristic_ITU2020, ColorPrimaries_ITU2020, CodingEquations_ITU2020_NCL);
const EnumeratedColorimetry EnumeratedColorimetry::COLOR_6("COLOR.6", TransferCharacteristic_SMPTEST2084, ColorPrimaries_P3D65, CodingEquations_ITU2020_NCL /* not used */);
const EnumeratedColorimetry EnumeratedColorimetry::COLOR_7("COLOR.7", TransferCharacteristic_SMPTEST2084, ColorPrimaries_ITU2020, CodingEquations_ITU2020_NCL);
const EnumeratedColorimetry EnumeratedColorimetry::COLOR_APP4_2("COLOR.APP4.2", TransferCharacteristic_CinemaMezzanine_DCDM, ColorPrimaries_CinemaMezzanine, CodingEquations_ITU2020_NCL /* not used */);

int main(int argc, const char* argv[]) {

    ASDCP::Result_t result = ASDCP::RESULT_OK;

    /* initialize command line options */

    boost::program_options::options_description cli_opts{ "Wraps JPEG 2000 codestreams (RGB only) into IMF Image Track File" };

    cli_opts.add_options()
        ("help", "Prints usage")
        ("fps", boost::program_options::value<ASDCP::Rational>()->default_value(ASDCP::EditRate_24), "Edit rate in the form of <numerator> '/' <denominator>, e.g. 24/1")
        ("aspect_ratio", boost::program_options::value<ASDCP::Rational>(), "Aspect ratio in the form of <numerator> '/' <denominator>, e.g. 16/9")
        ("format", boost::program_options::value<InputFormats>()->default_value(InputFormats::J2C), "Input codestream format\n"
            "  MJC: \t16-byte header followed by a sequence of J2C codestreams, each preceded by a 4-byte little-endian length\n"
            "  J2C: \tsingle JPEG 2000 codestream")
        ("assetid", boost::program_options::value<Kumu::UUID>(), "Asset UUID in hex notation, e.g. 8538b543169743dd9a08c6d8b4b1b7df")
        ("out", boost::program_options::value<std::string>()->required(), "Output file path")
        ("fake", boost::program_options::bool_switch()->default_value(false), "Generate fake input data")
        ("in", boost::program_options::value<std::string>(), "Input file path (or stdin if none is specified)")
        ("color", boost::program_options::value<std::string>()->default_value(EnumeratedColorimetry::COLOR_APP4_2.symbol()), EnumeratedColorimetry::usage().c_str())
        ("components", boost::program_options::value<ImageComponents>()->default_value(ImageComponents::XYZ), "Image components: RGB or YCbCr or XYZ")
        ("quantization", boost::program_options::value<Quantization>()->default_value(Quantization::QE_2), "Quantization: QE.1 or QE.2");

    boost::program_options::variables_map cli_args;

    try {

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cli_opts), cli_args);

        boost::program_options::notify(cli_args);

        /* display help options */

        if (cli_args.count("help")) {
            std::cout << cli_opts << "\n";
            return 1;
        }

        const ASDCP::Dictionary* g_dict = &ASDCP::DefaultSMPTEDict();

        if (!g_dict) {
            throw std::runtime_error("Cannot open SMPTE dictionary");
        }

        /* setup the input codestream sequence */

        std::unique_ptr<CodestreamSequence>  seq;

        if (cli_args["fake"].as<bool>()) {

            seq.reset(new FakeSequence());

        } else {

            FILE* f_in = NULL;

            if (cli_args.count("in") == 0) {

                /* open stdin in binary mode */

#ifdef WIN32

                int mode = _setmode(_fileno(stdin), O_BINARY);

if (mode == -1) {
    throw std::runtime_error("Cannot reopne stdout");
}

f_in = stdin;

#else

f_in = freopen(NULL, "rb", stdin);

#endif

            } else {

            f_in = fopen(cli_args["in"].as<std::string>().c_str(), "rb");

            }

            if (!f_in) {
                throw std::runtime_error("Cannot open input file");
            }

            switch (cli_args["format"].as<InputFormats>()) {

            case InputFormats::J2C:
                seq.reset(new J2CFile(f_in));
                break;

            case InputFormats::MJC:
                seq.reset(new MJCFile(f_in));
                break;

            }

        }

        /* Codestream frame buffer for the MXF writer */

        ASDCP::JP2K::FrameBuffer fb;

        /* MXF writer */

        AS_02::JP2K::MXFWriter writer;

        /* information about this software that will be written in the header metadata*/

        DCDM2IMFWriterInfo writer_info;

        writer_info.LabelSetType = ASDCP::LS_MXF_SMPTE;

        if (cli_args.count("assetid")) {

            const Kumu::UUID& uuid = cli_args["assetid"].as<ASDCP::UUID>();

            memcpy(writer_info.AssetUUID, uuid.Value(), uuid.Size());

        } else {

            Kumu::GenRandomUUID(writer_info.AssetUUID);

        }

        /* keep count of the number of frames written to the file */

        uint32_t frame_count = 0;

        while (seq->good()) {

            /* setup the frame buffer using the current codestream */

            seq->fill(fb);

            fb.PlaintextOffset(0);

            /* fill J2K picture descriptor */

            ASDCP::JP2K::PictureDescriptor pdesc;

            byte_t start_of_data;

            result = ASDCP::JP2K::ParseMetadataIntoDesc(fb, pdesc, &start_of_data);

            if (ASDCP_FAILURE(result)) {
                throw std::runtime_error(result.Message());
            }

            pdesc.EditRate = cli_args["fps"].as<ASDCP::Rational>();

            /* initialize header metadata if this is the first codestream */

            if (frame_count == 0) {

                /* build the essence descriptor */

                ASDCP::MXF::GenericPictureEssenceDescriptor* desc = NULL;

                /* initialize the J2K subdescriptor */

                ASDCP::MXF::JPEG2000PictureSubDescriptor *j2k_subdesc = new ASDCP::MXF::JPEG2000PictureSubDescriptor(g_dict);
                                
                /* determine pixel depth from the first component */

                if (!(pdesc.ImageComponents[0].Ssize == pdesc.ImageComponents[1].Ssize && pdesc.ImageComponents[1].Ssize == pdesc.ImageComponents[2].Ssize)) {
                    throw std::runtime_error("Not all components have equal pixel depth");
                }

                uint8_t pixel_depth = pdesc.ImageComponents[0].Ssize + 1;

                /* determine the color scheme */

                const EnumeratedColorimetry& color = EnumeratedColorimetry::fromString(cli_args["color"].as<std::string>());

                /* J2CLayout depends on the image components */

                std::array<uint8_t, ASDCP::MXF::RGBAValueLength> j2c_layout;
           
                if (cli_args["components"].as<ImageComponents>() == ImageComponents::YCbCr) {

                    /* YCbCr image */

                    if (pdesc.ImageComponents[1].YRsize != pdesc.ImageComponents[2].YRsize) {

                        throw std::runtime_error("Inconsistent subsampling");
                    }

                    if (cli_args["quantization"].as<Quantization>() != Quantization::QE_1) {

                        throw std::runtime_error("Quantization must be QE.1 for YCbCr images");

                    }

                    /* compute the J2C Layout */

                    j2c_layout = { 0x59, pixel_depth, 0x55, pixel_depth, 0x56, pixel_depth, 0x00 };

                    /* build the CDCI descriptor */

                    ASDCP::MXF::CDCIEssenceDescriptor *yuv_desc = new ASDCP::MXF::CDCIEssenceDescriptor(g_dict);

                    yuv_desc->CodingEquations = color.coding_equations().data();

                    yuv_desc->HorizontalSubsampling = pdesc.ImageComponents[1].YRsize;

                    yuv_desc->VerticalSubsampling = 1;

                    yuv_desc->ComponentDepth = pdesc.ImageComponents->Ssize + 1;

                    yuv_desc->ColorSiting = 0;

                    yuv_desc->WhiteReflevel = (2 << pdesc.ImageComponents->Ssize) - 21 * (2 << (pdesc.ImageComponents->Ssize - 8)); /* 2^A1 - 21*2^A1/2^8 */

                    yuv_desc->BlackRefLevel = 2 << (pdesc.ImageComponents->Ssize - 4);

                    yuv_desc->ColorRange = (2 << pdesc.ImageComponents->Ssize) - (2 << (pdesc.ImageComponents->Ssize - 3)) + 1; /* 2^A1 - 2^A1/2^3 + 1 */

                    desc = yuv_desc;

                } else {

                    /* RGB image */

                    ASDCP::MXF::RGBAEssenceDescriptor *rgba_desc = new ASDCP::MXF::RGBAEssenceDescriptor(g_dict);

                    if (pdesc.ImageComponents[1].YRsize == 2 || pdesc.ImageComponents[2].YRsize == 2) {

                        throw std::runtime_error("Components are sub-sampled but RGB images have been requested");

                    }

                    if (cli_args["quantization"].as<Quantization>() == Quantization::QE_1) {

                        rgba_desc->ComponentMaxRef = (2 << pdesc.ImageComponents->Ssize) - 21 * (2 << (pdesc.ImageComponents->Ssize - 8));
                        rgba_desc->ComponentMinRef = 2 << (pdesc.ImageComponents->Ssize - 4);

                    } else {

                        rgba_desc->ComponentMaxRef = (2 << pdesc.ImageComponents->Ssize) - 1;
                        rgba_desc->ComponentMinRef = 0;

                    }
                    
                    /* compute the J2C Layout */

                    if (cli_args["components"].as<ImageComponents>() == ImageComponents::XYZ) {

                        j2c_layout = { 0xd8, pixel_depth, 0xd9, pixel_depth, 0xda, pixel_depth, 0x00 };

                    } else {

                        j2c_layout = { 0x52, pixel_depth, 0x47, pixel_depth, 0x42, pixel_depth, 0x00 };

                    }

                    desc = rgba_desc;

                }

                /* set the J2CLayout */

                j2k_subdesc->J2CLayout.set(j2c_layout.data());

                /* fill the essence descriptor */

                result = ASDCP::JP2K_PDesc_to_MD(
                    pdesc,
                    *g_dict,
                    *desc,
                    *j2k_subdesc
                );

                if (ASDCP_FAILURE(result)) {
                    throw std::runtime_error(result.Message());
                }

                /* determine Picture Essence Coding Label */

                if (pdesc.ExtendedCapabilities.Pcap & 0x00020000) {

                    /* Part 15 codestream */

                    desc->PictureEssenceCoding = HTJ2KPictureCodingSchemeGeneric.data();

                } else {

                    /* Part 1 codestream */

                    std::map<int, std::pair<int, int>>::const_iterator ul_bytes = J2KPROFILE_UL_MAP.find(pdesc.Rsize);

                    if (ul_bytes != J2KPROFILE_UL_MAP.end()) {

                        /* It is an IMF profile */

                        std::array<uint8_t, 16> ul = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d, 0x04, 0x01, 0x02, 0x02, 0x03, 0x01, 0x00 /* placeholder*/, 0x00 /* placeholder*/ };

                        ul[14] = static_cast<uint8_t>(ul_bytes->second.first);
                        ul[15] = static_cast<uint8_t>(ul_bytes->second.second);

                        desc->PictureEssenceCoding = ul.data();

                    } else {

                        throw std::runtime_error("Supports only J2K IMF profiles or HTJ2K");

                    }

                }

                /* fill-in remaining common essence descriptor fields */

                desc->TransferCharacteristic = color.transfer_characteristic().data();

                desc->ColorPrimaries = color.color_primaries().data();

                desc->VideoLineMap = ASDCP::MXF::LineMapPair(0, 0);
             
                /* we do not know the container duration */

                desc->ContainerDuration.empty();

                ASDCP::MXF::FileDescriptor* essence_descriptor = static_cast<ASDCP::MXF::FileDescriptor*>(desc);

                if (ASDCP_FAILURE(result)) {
                    throw std::runtime_error(result.Message());
                }

                /* initialize the sub-descriptor container */

                ASDCP::MXF::InterchangeObject_list_t essence_sub_descriptors;

                essence_sub_descriptors.push_back(j2k_subdesc);

                /* initialize the MXF file */

                result = writer.OpenWrite(
                    cli_args["out"].as<std::string>(),
                    writer_info,
                    essence_descriptor,
                    essence_sub_descriptors,
                    cli_args["fps"].as<ASDCP::Rational>(),
                    16384,
                    AS_02::IndexStrategy_t::IS_FOLLOW,
                    60);

                if (ASDCP_FAILURE(result)) {
                    throw std::runtime_error(result.Message());
                }

            }

            /* write the codestream into a new frame */

            result = writer.WriteFrame(fb, NULL, NULL);

            if (ASDCP_FAILURE(result)) {
                throw std::runtime_error(result.Message());
            }

            /* move to the next codestream */

            seq->next();

            frame_count++;

        }

        result = writer.Finalize();

        if (ASDCP_FAILURE(result)) {
            throw std::runtime_error(result.Message());
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
