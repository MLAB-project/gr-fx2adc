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


#ifndef INCLUDED_FX2ADC_FX2ADC_H
#define INCLUDED_FX2ADC_FX2ADC_H

#include <fx2adc/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace fx2adc {

    /*!
     * \brief <+description of block+>
     * \ingroup fx2adc
     *
     */
    class FX2ADC_API fx2adc : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<fx2adc> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of fx2adc::fx2adc.
       *
       * To avoid accidental use of raw pointers, fx2adc::fx2adc's
       * constructor is in a private implementation
       * class. fx2adc::fx2adc::make is the public interface for
       * creating new instances.
       */
      static sptr make(int no_of_channels, std::string fw);
    };

  } // namespace fx2adc
} // namespace gr

#endif /* INCLUDED_FX2ADC_FX2ADC_H */

