/* -*- c++ -*- */

#define FX2ADC_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "fx2adc_swig_doc.i"

%{
#include "fx2adc/fx2adc.h"
%}


%include "fx2adc/fx2adc.h"
GR_SWIG_BLOCK_MAGIC2(fx2adc, fx2adc);
