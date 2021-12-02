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

#include "CodestreamSequence.h"
#include <stdexcept>

/* J2CFile */

J2CFile::J2CFile(FILE *fp,
                 std::vector<uint8_t>::size_type initial_buf_sz,
                 std::vector<uint8_t>::size_type read_buf_sz) :
    good_(true),
    codestream_(),
    file_paths_stack_(),
    initial_buf_sz_(initial_buf_sz),
    read_buf_sz_(read_buf_sz)
{
    this->_fill_from_fp(fp);
};

J2CFile::J2CFile(const std::vector<std::string>& file_paths,
    std::vector<uint8_t>::size_type initial_buf_sz,
    std::vector<uint8_t>::size_type read_buf_sz) :
    good_(true),
    codestream_(),
    file_paths_stack_(file_paths.rbegin(), file_paths.rend()),
    initial_buf_sz_(initial_buf_sz),
    read_buf_sz_(read_buf_sz)
{
    this->next();
};

void J2CFile::_fill_from_fp(FILE* fp) {

    this->codestream_.reserve(this->initial_buf_sz_);
    this->codestream_.resize(0);

    while (true) {

        std::vector<uint8_t>::size_type old_sz = this->codestream_.size();

        this->codestream_.resize(old_sz + this->read_buf_sz_);

        size_t sz = fread(this->codestream_.data() + old_sz, 1, this->read_buf_sz_, fp);

        if (sz != this->read_buf_sz_) {

            this->codestream_.resize(old_sz + sz);

            break;
        }
    }
}

void J2CFile::next() { 

    if (this->file_paths_stack_.size() == 0) {

        this->good_ = false;

        return;

    }

    FILE* fp = fopen(file_paths_stack_.back().c_str(), "rb");


    if (!fp) {
        throw std::runtime_error("Cannot open file: " + file_paths_stack_.back());
    }

    this->_fill_from_fp(fp);

    fclose(fp);

    file_paths_stack_.pop_back();

};

bool J2CFile::good() const { return this->good_; };

void J2CFile::fill(ASDCP::JP2K::FrameBuffer &fb)
{
  ASDCP::Result_t result = ASDCP::RESULT_OK;

  result = fb.SetData(this->codestream_.data(), (uint32_t)this->codestream_.size());

  if (ASDCP_FAILURE(result)) {
      throw std::runtime_error("Frame buffer allocation failed");
  }

  assert(ASDCP_SUCCESS(result));

  uint32_t sz = fb.Size((uint32_t)this->codestream_.size());

  if (sz != this->codestream_.size()) {
      throw std::runtime_error("Frame buffer resizing failed");
  }

};

/* MJCFile */

MJCFile::MJCFile(FILE *fp) : good_(true), codestream_len_(0), codestream_(), fp_(fp)
{

  uint8_t header[16];

  size_t sz = fread(header, 1, sizeof header, this->fp_);

  if (sz != sizeof header || header[0] != 'M' || header[1] != 'J' || header[2] != 'C' || header[3] != '2')
  {
    throw std::runtime_error("Bad MJC file");
  }

  this->is_cbr_ = header[15] & 4;

  this->next();
}

void MJCFile::next()
{

  size_t rd_sz;

  /* get codestream length */
  
  uint32_t len;
   
  if (!this->is_cbr_ || this->codestream_len_ == 0) {

    /* read the codestream length for every codestream if in vbr mode
     * otherwise read it from the first codestream only
     */

    uint8_t be_len[4];

    rd_sz = fread(be_len, 1, sizeof be_len, this->fp_);

    if (rd_sz != sizeof be_len) {
        this->good_ = false;
        return;
    }

    len = (be_len[0] << 24) + (be_len[1] << 16) + (be_len[2] << 8) + be_len[3];

    if (this->is_cbr_) {

      if (len == 0) {
        throw std::runtime_error("MJC CBR length is 0");
      }

      this->codestream_len_ = len;
    }

  } else {
  
    len = this->codestream_len_;

  }

  /* read codestream */

  this->codestream_.resize(len);

  rd_sz = fread(this->codestream_.data(), 1, len, this->fp_);

  if (rd_sz != len)
  {
    this->good_ = false;
    return;
  }
};

bool MJCFile::good() const { return this->good_; };

void MJCFile::fill(ASDCP::JP2K::FrameBuffer &fb)
{
  ASDCP::Result_t result = ASDCP::RESULT_OK;

  result = fb.SetData(this->codestream_.data(), (uint32_t)this->codestream_.size());

  if (ASDCP_FAILURE(result)) {
      throw std::runtime_error("Frame buffer allocation failed");
  }

  uint32_t sz = fb.Size((uint32_t)this->codestream_.size());

  if (sz != this->codestream_.size()) {
      throw std::runtime_error("Frame buffer resizing failed");
  }
};

/* FakeSequence */

FakeSequence::FakeSequence(uint32_t frame_count, uint32_t frame_size) :
  frame_count_(frame_count), cur_frame_(0), codestream_(frame_size)
{
  std::copy(CODESTREAM_HEADER_, CODESTREAM_HEADER_ + sizeof CODESTREAM_HEADER_, codestream_.begin());

  std::fill(codestream_.begin() + sizeof CODESTREAM_HEADER_, codestream_.end(), (uint8_t)0x00);
};

void FakeSequence::next()
{
  this->cur_frame_++;
};

bool FakeSequence::good() const { return this->cur_frame_ < this->frame_count_; };

void FakeSequence::fill(ASDCP::JP2K::FrameBuffer &fb)
{

  ASDCP::Result_t result = ASDCP::RESULT_OK;

  result = fb.SetData(this->codestream_.data(), (uint32_t)this->codestream_.size());

  assert(ASDCP_SUCCESS(result));

  uint32_t sz = fb.Size((uint32_t)this->codestream_.size());

  if (sz != this->codestream_.size()) {
      throw std::runtime_error("Frame buffer resizing failed");
  }
};


const uint8_t FakeSequence::CODESTREAM_HEADER_[236] = {
        0xFF, 0x4F, 0xFF, 0x51, 0x00, 0x2F, 0x40, 0x00, 0x00, 0x00, 0x0F, 0x00,
        0x00, 0x00, 0x08, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x08, 0x70, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0F, 0x01, 0x01, 0x0F, 0x01, 0x01,
        0x0F, 0x01, 0x01, 0xFF, 0x50, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x0C, 0xFF, 0x52, 0x00, 0x12, 0x01, 0x02, 0x00, 0x01, 0x01, 0x05, 0x03,
        0x03, 0x40, 0x01, 0x77, 0x88, 0x88, 0x88, 0x88, 0x88, 0xFF, 0x5C, 0x00,
        0x13, 0x20, 0x90, 0x98, 0x98, 0xA0, 0x98, 0x98, 0xA0, 0x98, 0x98, 0xA0,
        0x98, 0x98, 0x98, 0x90, 0x90, 0x98, 0xFF, 0x64, 0x00, 0x18, 0x00, 0x01,
        0x4B, 0x61, 0x6B, 0x61, 0x64, 0x75, 0x2D, 0x76, 0x78, 0x74, 0x37, 0x2E,
        0x31, 0x31, 0x2D, 0x42, 0x65, 0x74, 0x61, 0x34, 0xFF, 0x64, 0x00, 0x5C,
        0x00, 0x01, 0x4B, 0x64, 0x75, 0x2D, 0x4C, 0x61, 0x79, 0x65, 0x72, 0x2D,
        0x49, 0x6E, 0x66, 0x6F, 0x3A, 0x20, 0x6C, 0x6F, 0x67, 0x5F, 0x32, 0x7B,
        0x44, 0x65, 0x6C, 0x74, 0x61, 0x2D, 0x44, 0x28, 0x73, 0x71, 0x75, 0x61,
        0x72, 0x65, 0x64, 0x2D, 0x65, 0x72, 0x72, 0x6F, 0x72, 0x29, 0x2F, 0x44,
        0x65, 0x6C, 0x74, 0x61, 0x2D, 0x4C, 0x28, 0x62, 0x79, 0x74, 0x65, 0x73,
        0x29, 0x7D, 0x2C, 0x20, 0x4C, 0x28, 0x62, 0x79, 0x74, 0x65, 0x73, 0x29,
        0x0A, 0x2D, 0x31, 0x39, 0x32, 0x2E, 0x30, 0x2C, 0x20, 0x20, 0x33, 0x2E,
        0x37, 0x65, 0x2B, 0x30, 0x37, 0x0A, 0xFF, 0x90, 0x00, 0x0A, 0x00, 0x00,
        0x02, 0x2D, 0x2F, 0xD0, 0x00, 0x01, 0xFF, 0x93
};