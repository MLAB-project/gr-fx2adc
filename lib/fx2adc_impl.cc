/* -*- c++ -*- */
/* 
 * Copyright 2016 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>

#include <fx2adc/fx2adc.h>
#include <fx2pipe.h>
//#include "fx2adc_impl.h"

/* from usrp_source_impl.h */
static const pmt::pmt_t TIME_KEY = pmt::string_to_symbol("rx_time");
static const pmt::pmt_t RATE_KEY = pmt::string_to_symbol("rx_rate");
static const pmt::pmt_t FREQ_KEY = pmt::string_to_symbol("rx_freq");

namespace gr {
  namespace fx2adc {
    struct fx2pipe_bufreader {
      FX2Pipe &pipe;

      uint8_t *p;
      size_t len;

      fx2pipe_bufreader(FX2Pipe &pipe)
        : pipe(pipe), p(0), len(0)
      {}

      void pump() {
        if (pipe.read() < 0)
          throw std::runtime_error("fx2 read error");
        p = (uint8_t *) pipe.read_buf;
        len = pipe.read_length;
      }

      uint8_t getc() {
        if (len <= 0)
          pump();

        len--;
        return *p++;
      }
    };

    class fx2adc_impl : public fx2adc
    {
     private:
      int no_of_channels;
      std::string fw;
      bool synced;
      FX2Pipe fx2pipe;
      fx2pipe_bufreader reader;
      bool pps_status;

     public:
      fx2adc_impl(int no_of_channels, std::string fw);
      ~fx2adc_impl();

      bool start();
      bool stop();

      void look_for_pps(uint8_t *frame, int fi);

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

    float deserialize_sample(uint8_t *p, uint8_t mask, bool rl) {
      uint32_t smp = 0;

      for (int i = 0; i < 32; i++) {
        char b = p[i];
        if (((b & 0x80) != 0) != rl)
          throw std::runtime_error(":(");

        smp <<= 1;
        if (b & mask)
          smp |= 1;
      }
      
      /* TODO: is this the right way? */
      return ((float) ((int32_t) smp)) / (1 << 31);
    }

    fx2adc::sptr
    fx2adc::make(int no_of_channels, std::string fw)
    {
      return gnuradio::get_initial_sptr
        (new fx2adc_impl(no_of_channels, fw));
    }

    /*
     * The private constructor
     */
    fx2adc_impl::fx2adc_impl(int no_of_channels, std::string fw)
      : gr::sync_block("fx2adc",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(0, 1024, no_of_channels * sizeof(gr_complex))),
        no_of_channels(no_of_channels), synced(false), reader(fx2pipe), fw(fw)
    {}

    /*
     * Our virtual destructor.
     */
    fx2adc_impl::~fx2adc_impl()
    {
    }

    bool
    fx2adc_impl::start()
    {
      /*
        // Dump firmware config values: 
        fprintf(stderr,"Firmware config: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
          (unsigned int)p.fc.FC_DIR,   (unsigned int)p.fc.FC_IFCONFIG,
          (unsigned int)p.fc.FC_EPCFG, (unsigned int)p.fc.FC_EPFIFOCFG,
          (unsigned int)p.fc.FC_CPUCS );
      */

      /* These values were extracted from a running fx2pipe instance. */
      fx2pipe.fc.FC_DIR = 0x12;
      fx2pipe.fc.FC_IFCONFIG = 0xc3;
      fx2pipe.fc.FC_EPCFG = 0xe0;
      fx2pipe.fc.FC_EPFIFOCFG = 0x0d;
      fx2pipe.fc.FC_CPUCS = 0x12;

      fx2pipe.firmware_hex_path = fw.c_str();

      if (fx2pipe.start() < 0)
        throw std::runtime_error("fx2pipe initialization failed");

      return true;
    }

    bool
    fx2adc_impl::stop()
    {
      fx2pipe.stop();

      return true;
    }

    void
    fx2adc_impl::look_for_pps(uint8_t *frame, int fi)
    {
      for (int i = 0; i < 64; i++) {
        bool this_byte = (frame[i] & 0x40) != 0;

        /* look for ascending edge */
        if (this_byte && !pps_status) {
          struct timespec tp;
          clock_gettime(CLOCK_REALTIME, &tp);

          /* TODO */
          const pmt::pmt_t val = pmt::make_tuple(
            pmt::from_uint64(tp.tv_sec + (tp.tv_nsec > 900000000 ? 1 : 0)),
            //pmt::from_double(((double) tp.tv_nsec) / 1000000000)
            pmt::from_double(0.0)
          );
          add_item_tag(0, nitems_written(0) + fi, TIME_KEY, val);
        }

        pps_status = this_byte;
      }
    }

    int
    fx2adc_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
      gr_complex *out = (gr_complex *) output_items[0];

      if (!synced) {
        int i;

        for (int i = 0; i < 16; i++)
          reader.pipe.read();

        i = 0;
        while ((reader.getc() & 0x80) && (i < 256)) i++;
        if (i >= 256)
          throw std::runtime_error("fx2adc: no signal on LRCLK");

        i = 0;
        while ((!(reader.getc() & 0x80)) && (i < 256)) i++;
        if (i >= 256)
          throw std::runtime_error("fx2adc: no signal on LRCLK");

        for (i = 0; i < 31; i++)
          if (!(reader.getc() & 0x80))
            return 0;

        for (i = 0; i < 32; i++)
          if (reader.getc() & 0x80)
            return 0;

        for (i = 0; i < 32; i++)
          if (!(reader.getc() & 0x80))
            return 0;

        synced = true;
        pps_status = false;
        fprintf(stderr, "fx2adc: synced!\n");
      }

      int fi = 0;

      if (reader.len < 64) {
        uint8_t frame[64];
        for (int i = 0; i < 64; i++)
          frame[i] = reader.getc();

        for (int ch = 0; ch < no_of_channels; ch++) {
          out[no_of_channels * fi + ch] = gr_complex(
            deserialize_sample(frame, 0x01 << ch, false),
            deserialize_sample(frame + 32, 0x01 << ch, true)
          );
        }
        look_for_pps(frame, fi);

        fi++;
      }

      {
        int nframes = std::min((int) reader.len / 64, noutput_items - fi);

        for (int i = 0; i < nframes; i++) {
          for (int ch = 0; ch < no_of_channels; ch++) {
            out[no_of_channels * fi + ch] = gr_complex(
              deserialize_sample(reader.p + (64 * i) + 0, 0x01 << ch, false),
              deserialize_sample(reader.p + (64 * i) + 32, 0x01 << ch, true)
            );
          }
          look_for_pps(reader.p + (64 * i), fi);
  
          fi++;
        }
  
        reader.p += nframes * 64;
        reader.len -= nframes * 64;
      }

      return fi;
    }

  } /* namespace fx2adc */
} /* namespace gr */

