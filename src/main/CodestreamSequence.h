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

#ifndef COM_SANDFLOW_CODESTREAMSEQUENCE_H
#define COM_SANDFLOW_CODESTREAMSEQUENCE_H

#include <vector>
#include <list>
#include <AS_DCP.h>

class CodestreamSequence {

public:

    virtual void next() = 0;
    virtual bool good() const = 0;
    virtual void fill(ASDCP::JP2K::FrameBuffer& fb) = 0;
    virtual ~CodestreamSequence() {};
};

class J2CFile : public CodestreamSequence {

public:

    J2CFile(FILE* fp,
        std::vector<uint8_t>::size_type initial_buf_sz = 50 * 1024 * 1024,
        std::vector<uint8_t>::size_type read_buf_sz = 64 * 1024);

    J2CFile(const std::vector<std::string>& file_paths,
        std::vector<uint8_t>::size_type initial_buf_sz = 50 * 1024 * 1024,
        std::vector<uint8_t>::size_type read_buf_sz = 64 * 1024);

    virtual void next();

    virtual bool good() const;

    virtual void fill(ASDCP::JP2K::FrameBuffer& fb);

protected:

    bool good_;
    std::vector<uint8_t> codestream_;
    std::vector<std::string> file_paths_stack_;
    std::vector<uint8_t>::size_type initial_buf_sz_;
    std::vector<uint8_t>::size_type read_buf_sz_;

    void _fill_from_fp(FILE* fp);
};

class MJCFile : public CodestreamSequence {

public:

    MJCFile(FILE* fp);

    virtual void next();

    virtual bool good() const;

    virtual void fill(ASDCP::JP2K::FrameBuffer& fb);

protected:

    bool good_;
    std::vector<uint8_t> codestream_;
    FILE* fp_;

};

class FakeSequence : public CodestreamSequence {

public:

    FakeSequence(uint32_t frame_count = 360, uint32_t frame_size = 5 * 1024 * 1024);

    virtual void next();

    virtual bool good() const;

    virtual void fill(ASDCP::JP2K::FrameBuffer& fb);

protected:

    uint32_t frame_count_;
    uint32_t cur_frame_;
    std::vector<uint8_t> codestream_;

    const static uint8_t CODESTREAM_HEADER_[236];


};

#endif