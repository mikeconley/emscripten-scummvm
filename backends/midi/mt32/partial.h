/* Copyright (c) 2003-2004 Various contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MT32EMU_PARTIAL_H
#define MT32EMU_PARTIAL_H

namespace MT32Emu {

class Synth;
struct NoteLookup;

enum EnvelopeType {
	EnvelopeType_amp = 0,
	EnvelopeType_filt = 1,
	EnvelopeType_pitch = 2
};

struct envstatus {
	Bit32s envpos;
	Bit32s envstat;
	Bit32s envbase;
	Bit32s envdist;
	Bit32s envsize;

	bool sustaining;
	bool decaying;
	Bit32s prevlevel;

	Bit32s counter;
	Bit32s count;
};

// Class definition of MT-32 partials.  32 in all.
class Partial {
private:
	Synth *synth; // Only used for sending debug output

	int ownerPart; // -1 if unassigned
	int mixType;
	int structurePosition; // 0 or 1 of a structure pair
	bool useNoisePair;

	Bit16s myBuffer[MAX_SAMPLE_OUTPUT];

	bool play;

	// Keyfollowed note value
	int noteVal;
	NoteLookup *noteLookup; // Lookup stuff for this noteVal

	int keyVal;
	NoteLookup *keyLookup;

	// Keyfollowed filter values
	int realVal;
	int filtVal;

	envstatus envs[3];

	int pulsewidth;

	Bit32u lfoPos;
	soundaddr partialOff;

	Bit32u ampEnvCache;
	Bit32u pitchEnvCache;

	float history[32];

	bool pitchSustain;

	int loopPos;

	dpoly *poly;

	int bendShift;

	Bit16s *mixBuffers(Bit16s * buf1, Bit16s * buf2, int len);
	Bit16s *mixBuffersRingMix(Bit16s * buf1, Bit16s * buf2, int len);
	Bit16s *mixBuffersRing(Bit16s * buf1, Bit16s * buf2, int len);
	void mixBuffersStereo(Bit16s * buf1, Bit16s * buf2, Bit16s * outBuf, int len);

	Bit32s getFiltEnvelope();
	Bit32s getAmpEnvelope();
	Bit32s getPitchEnvelope();

	void initKeyFollow(int freqNum);

public:
	PatchCache *patchCache;
	PatchCache cachebackup;

	Partial *pair;
	bool alreadyOutputed;
	Bit64s age;

	Partial(Synth *synth);
	~Partial();

	int getOwnerPart();
	bool isActive();
	void activate(int part);
	void deactivate(void);
	void startPartial(dpoly *usePoly, PatchCache *useCache, Partial *pairPartial);
	void startDecay(EnvelopeType envnum, Bit32s startval);
	void startDecayAll();
	void setBend(float factor);
	bool shouldReverb();

	// Returns true only if data written to buffer
	// This function (unlike the one below it) returns processed stereo samples
	// made from combining this single partial with its pair, if it has one.
	bool produceOutput(Bit16s * partialBuf, long length);

	// This function produces mono sample output using the partial's private internal buffer
	Bit16s *generateSamples(long length);
};

}

#endif
