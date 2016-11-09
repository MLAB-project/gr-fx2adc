/*
 * usb_io/fx2pipe.cc
 * 
 * Cypress FX2 pipe IO main class. 
 * 
 * Copyright (c) 2006 by Wolfgang Wieser ] wwieser (a) gmx <*> de [ 
 * 
 * This file may be distributed and/or modified under the terms of the 
 * GNU General Public License version 2 as published by the Free Software 
 * Foundation. (See COPYING.GPL for details.)
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 */

#include "fx2pipe.h"
//#include "../lib/databuffer.h"

#include <stdio.h>
#include <sched.h>
#include <errno.h>
//#include <unistd.h>
//#include <time.h>
//#include <sys/time.h>

#include <assert.h>


// Initialize static data: 
URBCache FX2Pipe::MyURB::urb_cache(/*max_elem=*/256,
	/*elem_size=*/sizeof(FX2Pipe::MyURB));


int FX2Pipe::_ConnectAndInitUSB()
{
	// Be sure: 
	_CleanupUSB();
	
	// Connect and init USB device. 
// FIXME: Support bus/device. 
	FX2USBDevice::ErrorCode ec=FX2USBDevice::connect(
		/*initial_vendor=*/search_vid,/*initial_product=*/search_pid,
			/*n_th=*/n_th_usb_dev,
		/*firmware_path=*/firmware_hex_path,
		/*firmware_buf=*/NULL,
		(const char*)&fc,sizeof(FirwareConfig),FirmwareConfigAdr);
	if(ec)
	{
		fprintf(stderr,"FX2 initialisation failure (ec=%d)\n",ec);
		return(1);
	}
	
	const int rx_interface=0;
	const int rx_altinterface=1;  // alt interface: 1: bulk, 2: interrupt (?), see TRM
	
	ec=FX2USBDevice::claim(rx_interface,rx_altinterface);
	if(ec)
	{
		fprintf(stderr,"Failed to claim interface %d,%d: ec=%d\n",
			rx_interface,rx_altinterface,ec);
		return(2);
	}
	
	return(0);
}


void FX2Pipe::_CleanupUSB()
{
	// NOTE: This function must be safe to be called several times 
	//       even when the USB is already shut down. 
	FX2USBDevice::disconnect();
	FX2Pipe::MyURB::urb_cache.clear();
}


int FX2Pipe::_SubmitOneURB()
{
	if(stdio_eof)
	{  return(0);  }
	
	size_t iobs=io_block_size;
	if(transfer_limit>=0 && transfer_limit-submitted_bytes<int64(iobs))
	{
		assert(transfer_limit>=submitted_bytes);
		iobs=transfer_limit-submitted_bytes;
		
		stdio_eof=2;
	}
	
	if(!iobs)
	{  return(0);  }
	
	MyURB *u = new MyURB(dir<0 ? 0x86U/*EP6 IN*/ : 0x02U/*EP2 OUT*/);
// FIXME: In the OUT case we don't need to allocate buffers all the time 
//        since the content is copied by the kernel upon submission time. 
// FIXME: This means we don't need to zero out the buffer each time if 
//        no_stdio is set. 
	u->AllocBuffer(iobs);
	
	if(u->buffer_length)
	{
		ErrorCode ec=SubmitURB(u);
		if(ec)
		{
			// This is a problem because we hereby lose one in the pipeline. 
			// Let's see if this turns out to be a problem. 
			fprintf(stderr,"OOPS: URB submission failed (ec=%d)\n",ec);
			return(1);
		}
		
		submitted_bytes+=u->buffer_length;
	}
	else
	{  delete u;  }
	
	return(0);
}


int FX2Pipe::_SubmitInitialURBs()
{
	if(!udh)  return(2);
	
	// We submit some URBs with specified iobs. 
	
	fprintf(stderr,"Submitting max. %d URBs to fill pipeline... ",
		pipeline_size);
	
	gettimeofday(&starttime,NULL);
	last_update_time=starttime;
	
	for(int i=0; i<pipeline_size; i++)
	{
		if(_SubmitOneURB())
		{  return(1);  }
		if(stdio_eof)  break;
	}
	
	fprintf(stderr,"%d submitted\n",npending);
	
	return(0);
}


void FX2Pipe::_CancelAllPendingDataURBs()
{
	for(URB *_u=pending.last(); _u; )
	{
		URB *u=_u;
		_u=_u->prev;
		
		ErrorCode ec=CancelURB(u);
		// We get errno=EINVAL when cancelling an URB which is already 
		// completed but not yet reaped. Ignore that. 
		if(ec && ec!=EINVAL)
		{  fprintf(stderr,"Failed to cancel (data) URB: ec=%d (%s)\n",
			ec,strerror(ec));  }
	}
}


void FX2Pipe::_DisplayTransferStatistics(const timeval *endtime,int final)
{
	long long msec = 
		1000LL*(endtime->tv_sec-starttime.tv_sec) + 
		       (endtime->tv_usec-starttime.tv_usec+500)/1000;
	fprintf(stderr,
		"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
		"fx2pipe: %lld bytes in %lld.%03ds",
		(long long)transferred_bytes,
		(long long)(msec/1000),(int)(msec%1000));
	if(msec>10)
	{
		double bps = double(transferred_bytes)*1000.0 / double(msec);
		fprintf(stderr,
			" (avg %ld kb/s",
			(long)(bps/1024.0+0.5));
	}
	if(final)
	{  fprintf(stderr,")\n");  }
	else
	{
		long lu_msec = 1000L*(endtime->tv_sec-last_update_time.tv_sec) + 
			(endtime->tv_usec-last_update_time.tv_usec+500)/1000;
		long lu_bytes = (long)(transferred_bytes-last_update_transferred);
		double lu_bps = double(lu_bytes)*1000.0 / double(lu_msec);
		fprintf(stderr,
			", curr %ld kb/s)",
			(long)(lu_bps/1024.0+0.5));
	}
}

int FX2Pipe::start()
{
	fprintf(stderr,"IO loop running...\n");

	// LibUSB initialisation: 
	usb_init();
    usb_find_busses();
    usb_find_devices();
	
	if (_ConnectAndInitUSB())
		return -1;

	if (_SubmitInitialURBs())
		return -1;

	read_buf=malloc(io_block_size);
	read_length=io_block_size;

	/*
	// Set scheduler. 
	if(schedule_policy!=SCHED_OTHER)
	{
		// Change scheduler. 
		struct sched_param sp;
		memset(&sp,0,sizeof(sp));
		sp.sched_priority=schedule_priority;
		int rv=sched_setscheduler(0,schedule_policy,&sp);
		if(rv)
		{
			fprintf(stderr,"Failed to use realtime scheduling: %s\n",
				strerror(rv));
			schedule_policy=SCHED_OTHER;
		}
	}
	*/

	return 0;
}

int FX2Pipe::read()
{
	while(true)
	{
		ErrorCode ec = ProcessEvents(-1);

		switch(ec)
		{
			case ECTimeout:
				break;
			case ECUserQuit:
				return 0;
				break;
			case ECUserQuitFatal:
			case ECNotConnected:
			case ECNoURBAvail:
				return -1;
				break;
			default:
				fprintf(stderr,"OOPS: ProcessEvents: ec=%d\n",ec);
				// FIXME: What should we do???
				break;
		}
		
		if(npending<pipeline_size && !stdio_eof)
		{  fprintf(stderr,"OOPS: npending=%d<%d=pipeline_size. Lost URB??\n",
			npending,pipeline_size);  }
	}
}

void FX2Pipe::stop()
{
	gettimeofday(&endtime,NULL);

	fprintf(stderr,"IO loop exited\n");
	
	_CleanupUSB();
	
	_DisplayTransferStatistics(&endtime,1);
}

WWUSBDevice::ErrorCode FX2Pipe::URBNotify(URB *_u)
{
	MyURB *u=static_cast<MyURB*>(_u);
	if(u->actual_length!=u->buffer_length 
		|| u->status || u->error_count || u->cancelled)
	{
		if(!(u->cancelled==2 && u->status==-ENOENT))
		{  fprintf(stderr,"HMM: URB(%d): blen=%d/%d, status=%d, "
			"error count=%d, cancelled=%d\n",
			u->urb_serial,u->actual_length,u->buffer_length,
			u->status,u->error_count,u->cancelled);  }
	}
	
	// Simply ignore cancelled URBs. 
	if(u->cancelled)
	{  return(ECSuccess);  }
	
	if(u->status==-EPROTO)
	{
		if(++successive_error_urbs>pipeline_size+4)
		{
			fprintf(stderr,"Giving up after %d reaped URBs with errors.\n",
				successive_error_urbs);
			// Seems like it does not make sense to go on here. 
			return(ECUserQuitFatal);  // User quit. 
		}
	}
	else
	{  successive_error_urbs=0;  }
	
	// FIXME: We don't handle frames with protocol error yet. 
	if(u->status==0)
	{
		if(dir<0)  // Read in from USB. 
		{
			transferred_bytes+=u->actual_length;
			void *prior_display_buf=read_buf;

			read_buf=u->buffer;
			read_length=u->actual_length;

			u->buffer=prior_display_buf;
		}
		else //if(dir>0)  // Write out to USB. 
		{
			// Data successfully written over USB. 
			transferred_bytes+=u->actual_length;
		}
		
		//write(2,".",1);
	}
	
	{
		timeval curr_time;
		gettimeofday(&curr_time,NULL);
		long msec = 1000L*(curr_time.tv_sec-last_update_time.tv_sec) + 
			(curr_time.tv_usec-last_update_time.tv_usec+500)/1000;
		if(msec>250)
		{
			_DisplayTransferStatistics(&curr_time,0);
			last_update_time=curr_time;
			last_update_transferred=transferred_bytes;
		}
	}
	
	/*
	if(caught_sigint)
	{  return(ECUserQuit);  }
	*/
	
	// For each reaped URB, we submit a new one to keep things running. 
	if(_SubmitOneURB())
	{  return(ECUserQuit);  }
	
	//return(ECSuccess);
	return ECUserQuit;
}


void FX2Pipe::DeleteURB(URB *_u)
{
	MyURB *u=static_cast<MyURB*>(_u);
	delete u;
}


FX2Pipe::FX2Pipe() : 
	FX2USBDevice(),
	successive_error_urbs(0),
	//slurped_bytes(0),
	submitted_bytes(0),
	last_update_transferred(0),
	transferred_bytes(0),
	stdio_eof(0),
	n_th_usb_dev(0),
	search_vid(-1),
	search_pid(-1),
	transfer_limit(-1),
	io_block_size(16384),
	pipeline_size(16),
	dir(-1),
	no_stdio(0),
	schedule_policy(SCHED_OTHER),
	schedule_priority(0),
	firmware_hex_path(NULL)
{
	memset(&starttime,0,sizeof(starttime));
	memset(&endtime,0,sizeof(endtime));
	memset(&last_update_time,0,sizeof(last_update_time));
}

FX2Pipe::~FX2Pipe()
{
	_CleanupUSB();
}

//------------------------------------------------------------------------------

void FX2Pipe::MyURB::AllocBuffer(size_t size)
{
	buffer=malloc(size);
	if(!buffer)
	{  fprintf(stderr,"URB buffer allocation failure (%u bytes)\n",size);
		abort();  }
	buffer_length=size;
}

FX2Pipe::MyURB::MyURB(uchar dir_ep,uchar _type) : 
	FX2USBDevice::URB(dir_ep,_type)
{
}

FX2Pipe::MyURB::~MyURB()
{
	if(buffer)
	{  free(buffer);  buffer=NULL;  }
}
