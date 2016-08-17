/*
 * Copyright (C) 2005-2006 WIS Technologies International Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and the associated README documentation file (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "WISH264VideoServerMediaSubsession.hh"
#include <H264VideoRTPSink.hh>
#include "WNCH264VideoStreamDiscreteFramer.hh"

WISH264VideoServerMediaSubsession * WISH264VideoServerMediaSubsession
::createNew(UsageEnvironment& env, WISInput& wisInput, unsigned streamId, unsigned estimatedBitrate) {
  return new WISH264VideoServerMediaSubsession(env, wisInput, streamId, estimatedBitrate);
}

WISH264VideoServerMediaSubsession
::WISH264VideoServerMediaSubsession(UsageEnvironment& env, WISInput& wisInput, unsigned streamId,
				     unsigned estimatedBitrate)
  : WISServerMediaSubsession(env, wisInput, estimatedBitrate), 
  	fOurStreamId(streamId) {
}

WISH264VideoServerMediaSubsession::~WISH264VideoServerMediaSubsession() {
}

static void afterPlayingDummy(void* clientData) {
  WISH264VideoServerMediaSubsession * subsess
    = (WISH264VideoServerMediaSubsession *) clientData;
  // Signal the event loop that we're done:
  subsess->envir() << "afterPlayingDummy\n";
  subsess->setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  WISH264VideoServerMediaSubsession * subsess
    = (WISH264VideoServerMediaSubsession *) clientData;
  subsess->checkForAuxSDPLine1();
}

void WISH264VideoServerMediaSubsession::checkForAuxSDPLine1() {
  if (fDummyRTPSink->auxSDPLine() != NULL) {
    // Signal the event loop that we're done:
    setDoneFlag();
  } else {
    // try again after a brief delay:
    int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			      (TaskFunc*)checkForAuxSDPLine, this);
  }
}

char const* WISH264VideoServerMediaSubsession
::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  // Note: For MPEG-4 video buffer, the 'config' information isn't known
  // until we start reading the Buffer.  This means that "rtpSink"s
  // "auxSDPLine()" will be NULL initially, and we need to start reading
  // data from our buffer until this changes.
  envir() << "WISH264VideoServerMediaSubsession::getAuxSDPLine\n";
  fDummyRTPSink = rtpSink;
	
  // Start reading the buffer:
  fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);
  
  // Check whether the sink's 'auxSDPLine()' is ready:
  checkForAuxSDPLine(this);
	    
  fDoneFlag = 0;
  envir().taskScheduler().doEventLoop(&fDoneFlag);
    
  char const* auxSDPLine = fDummyRTPSink->auxSDPLine(); 
  return auxSDPLine;
  //return NULL;
}

FramedSource* WISH264VideoServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{
  	estBitrate = fEstimatedKbps;
  	// Create a framer for the Video Elementary Stream:
  	envir() << "Create a framer for the Video Elementary Stream:" << fOurStreamId << "\n";
  	return WNCH264VideoStreamDiscreteFramer::createNew(envir(), fWISInput.videoSource(fOurStreamId));
}

RTPSink* WISH264VideoServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
//  setVideoRTPSinkBufferSize();
  return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
