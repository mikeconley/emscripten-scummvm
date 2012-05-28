/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/archive.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "graphics/surface.h"
#include "audio/decoders/raw.h"

#include "sci/resource.h"
#include "sci/util.h"
#include "sci/sound/audio.h"
#include "sci/video/robot_decoder.h"

namespace Sci {

// TODO:
// - Positioning
// - Proper handling of frame scaling - scaled frames look squashed
//   (probably because both dimensions should be scaled)
// - Transparency support
// - Timing - the arbitrary 100ms delay between each frame is not quite right
// - Proper handling of sound chunks in some cases, so that the frame size
//   table can be ignored (it's only used to determine the correct sound chunk
//   size at the moment, cause it can be wrong in some cases)
// - Fix audio "hiccups" - probably data that shouldn't be in the audio frames


// Some non technical information on robot files, from an interview with
// Greg Tomko-Pavia of Sierra On-Line
// Taken from http://anthonylarme.tripod.com/phantas/phintgtp.html
//
// (...) What we needed was a way of playing video, but have it blend into
// normal room art instead of occupying its own rectangular area. Room art
// consists of a background pic overlaid with various animating cels
// (traditional lingo: sprites). The cels each have a priority that determines
// who is on top and who is behind in the drawing order. Cels are read from
// *.v56 files (another proprietary format). A Robot is video frames with
// transparent background including priority and x,y information. Thus, it is
// like a cel, except it comes from an RBT - not a v56. Because it blends into
// our graphics engine, it looks just like a part of the room. A RBT can move
// around the screen and go behind other objects. (...)

#ifdef ENABLE_SCI32

enum robotPalTypes {
	kRobotPalVariable = 0,
	kRobotPalConstant = 1
};

RobotDecoder::RobotDecoder(Audio::Mixer *mixer, bool isBigEndian) {
	_surface = 0;
	_width = 0;
	_height = 0;
	_fileStream = 0;
	_audioStream = 0;
	_dirtyPalette = false;
	_pos = Common::Point(0, 0);
	_mixer = mixer;
	_isBigEndian = isBigEndian;
}

RobotDecoder::~RobotDecoder() {
	close();
}

bool RobotDecoder::load(GuiResourceId id) {
	Common::String fileName = Common::String::format("%d.rbt", id);
	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(fileName);

	if (!stream) {
		warning("Unable to open robot file %s", fileName.c_str());
		return false;
	}

	return loadStream(stream);
}

bool RobotDecoder::loadStream(Common::SeekableReadStream *stream) {
	close();

	_fileStream = new Common::SeekableSubReadStreamEndian(stream, 0, stream->size(), _isBigEndian, DisposeAfterUse::YES);
	_surface = new Graphics::Surface();

	readHeaderChunk();

	// There are several versions of robot files, ranging from 3 to 6.
	// v3: no known examples
	// v4: PQ:SWAT demo
	// v5: SCI2.1 and SCI3 games
	// v6: SCI3 games
	if (_header.version < 4 || _header.version > 6)
		error("Unknown robot version: %d", _header.version);

	if (_header.hasSound) {
		_audioStream = Audio::makeQueuingAudioStream(11025, false);
		_mixer->playStream(Audio::Mixer::kMusicSoundType, &_audioHandle, _audioStream, -1, getVolume(), getBalance());
	}

	readPaletteChunk(_header.paletteDataSize);
	readFrameSizesChunk();
	calculateVideoDimensions();
	_surface->create(_width, _height, Graphics::PixelFormat::createFormatCLUT8());

	return true;
}

void RobotDecoder::readHeaderChunk() {
	// Header (60 bytes)
	_fileStream->skip(6);
	_header.version = _fileStream->readUint16();
	_header.audioChunkSize = _fileStream->readUint16();
	_header.audioSilenceSize = _fileStream->readUint16();
	_fileStream->skip(2);
	_header.frameCount = _fileStream->readUint16();
	_header.paletteDataSize = _fileStream->readUint16();
	_header.unkChunkDataSize = _fileStream->readUint16();
	_fileStream->skip(5);
	_header.hasSound = _fileStream->readByte();
	_fileStream->skip(34);

	// Some videos (e.g. robot 1305 in Phantasmagoria and
	// robot 184 in Lighthouse) have an unknown chunk before
	// the palette chunk (probably used for sound preloading).
	// Skip it here.
	if (_header.unkChunkDataSize)
		_fileStream->skip(_header.unkChunkDataSize);
}

void RobotDecoder::readPaletteChunk(uint16 chunkSize) {
	byte *paletteData = new byte[chunkSize];
	_fileStream->read(paletteData, chunkSize);

	// SCI1.1 palette
	byte palFormat = paletteData[32];
	uint16 palColorStart = paletteData[25];
	uint16 palColorCount = READ_SCI11ENDIAN_UINT16(paletteData + 29);

	int palOffset = 37;
	memset(_palette, 0, 256 * 3);

	for (uint16 colorNo = palColorStart; colorNo < palColorStart + palColorCount; colorNo++) {
		if (palFormat == kRobotPalVariable)
			palOffset++;
		_palette[colorNo * 3 + 0] = paletteData[palOffset++];
		_palette[colorNo * 3 + 1] = paletteData[palOffset++];
		_palette[colorNo * 3 + 2] = paletteData[palOffset++];
	}

	_dirtyPalette = true;
	delete[] paletteData;
}


void RobotDecoder::readFrameSizesChunk() {
	// The robot video file contains 2 tables, with one entry for each frame:
	// - A table containing the size of the image in each video frame
	// - A table containing the total size of each video frame.
	// In v5 robots, the tables contain 16-bit integers, whereas in v6 robots,
	// they contain 32-bit integers.

	_frameTotalSize = new uint32[_header.frameCount];

	// TODO: The table reading code can probably be removed once the
	// audio chunk size is figured out (check the TODO inside processNextFrame())
#if 0
	// We don't need any of the two tables to play the video, so we ignore
	// both of them.
	uint16 wordSize = _header.version == 6 ? 4 : 2;
	_fileStream->skip(_header.frameCount * wordSize * 2);
#else
	switch (_header.version) {
	case 4:
	case 5:		// sizes are 16-bit integers
		// Skip table with frame image sizes, as we don't need it
		_fileStream->skip(_header.frameCount * 2);
		for (int i = 0; i < _header.frameCount; ++i)
			_frameTotalSize[i] = _fileStream->readUint16();
		break;
	case 6:		// sizes are 32-bit integers
		// Skip table with frame image sizes, as we don't need it
		_fileStream->skip(_header.frameCount * 4);
		for (int i = 0; i < _header.frameCount; ++i)
			_frameTotalSize[i] = _fileStream->readUint32();
		break;
	default:
		error("Can't yet handle index table for robot version %d", _header.version);
	}
#endif

	// 2 more unknown tables
	_fileStream->skip(1024 + 512);

	// Pad to nearest 2 kilobytes
	uint32 curPos = _fileStream->pos();
	if (curPos & 0x7ff)
		_fileStream->seek((curPos & ~0x7ff) + 2048);
}

void RobotDecoder::calculateVideoDimensions() {
	// This is an O(n) operation, as each frame has a different size.
	// We need to know the actual frame size to have a constant video size.
	uint32 pos = _fileStream->pos();

	for (uint32 curFrame = 0; curFrame < _header.frameCount; curFrame++) {
		_fileStream->skip(4);
		uint16 frameWidth = _fileStream->readUint16();
		uint16 frameHeight = _fileStream->readUint16();
		if (frameWidth > _width)
			_width = frameWidth;
		if (frameHeight > _height)
			_height = frameHeight;
		_fileStream->skip(_frameTotalSize[curFrame] - 8);
	}

	_fileStream->seek(pos);
}

const Graphics::Surface *RobotDecoder::decodeNextFrame() {
	// Read frame image header (24 bytes)
	_fileStream->skip(3);
	byte frameScale = _fileStream->readByte();
	uint16 frameWidth = _fileStream->readUint16();
	uint16 frameHeight = _fileStream->readUint16();
	_fileStream->skip(4); // unknown, almost always 0
	uint16 frameX = _fileStream->readUint16();
	uint16 frameY = _fileStream->readUint16();
	// TODO: In v4 robot files, frameX and frameY have a different meaning.
	// Set them both to 0 for v4 for now, so that robots in PQ:SWAT show up
	// correctly.
	if (_header.version == 4)
		frameX = frameY = 0;
	uint16 compressedSize = _fileStream->readUint16();
	uint16 frameFragments = _fileStream->readUint16();
	_fileStream->skip(4); // unknown
	uint32 decompressedSize = frameWidth * frameHeight * frameScale / 100;
	// FIXME: A frame's height + position can go off limits... why? With the
	// following, we cut the contents to fit the frame
	uint16 scaledHeight = CLIP<uint16>(decompressedSize / frameWidth, 0, _height - frameY);
	// FIXME: Same goes for the frame's width + position. In this case, we
	// modify the position to fit the contents on screen.
	if (frameWidth + frameX > _width)
		frameX = _width - frameWidth;
	assert (frameWidth + frameX <= _width && scaledHeight + frameY <= _height);

	DecompressorLZS lzs;
	byte *decompressedFrame = new byte[decompressedSize];
	byte *outPtr = decompressedFrame;

	if (_header.version == 4) {
		// v4 has just the one fragment, it seems, and ignores the fragment count
		Common::SeekableSubReadStream fragmentStream(_fileStream, _fileStream->pos(), _fileStream->pos() + compressedSize);
		lzs.unpack(&fragmentStream, outPtr, compressedSize, decompressedSize);
	} else {
		for (uint16 i = 0; i < frameFragments; ++i) {
			uint32 compressedFragmentSize = _fileStream->readUint32();
			uint32 decompressedFragmentSize = _fileStream->readUint32();
			uint16 compressionType = _fileStream->readUint16();

			if (compressionType == 0) {
				Common::SeekableSubReadStream fragmentStream(_fileStream, _fileStream->pos(), _fileStream->pos() + compressedFragmentSize);
				lzs.unpack(&fragmentStream, outPtr, compressedFragmentSize, decompressedFragmentSize);
			} else if (compressionType == 2) {	// untested
				_fileStream->read(outPtr, compressedFragmentSize);
			} else {
				error("Unknown frame compression found: %d", compressionType);
			}

			outPtr += decompressedFragmentSize;
		}
	}

	// Copy over the decompressed frame
	byte *inFrame = decompressedFrame;
	byte *outFrame = (byte *)_surface->pixels;

	// Black out the surface
	memset(outFrame, 0, _width * _height);

	// Move to the correct y coordinate
	outFrame += _width * frameY;

	for (uint16 y = 0; y < scaledHeight; y++) {
		memcpy(outFrame + frameX, inFrame, frameWidth);
		inFrame += frameWidth;
		outFrame += _width;
	}

	delete[] decompressedFrame;

	// +1 because we start with frame number -1
	uint32 audioChunkSize = _frameTotalSize[_curFrame + 1] - (24 + compressedSize);

// TODO: The audio chunk size below is usually correct, but there are some
// exceptions (e.g. robot 4902 in Phantasmagoria, towards its end)
#if 0
	// Read frame audio header (14 bytes)
	_fileStream->skip(2); // buffer position
	_fileStream->skip(2); // unknown (usually 1)
	_fileStream->skip(2); /*uint16 audioChunkSize = _fileStream->readUint16() + 8;*/
	_fileStream->skip(2);
#endif

	// Queue the next audio frame
	// FIXME: For some reason, there are audio hiccups/gaps
	if (_header.hasSound) {
		_fileStream->skip(8);	// header
		_audioStream->queueBuffer(g_sci->_audio->getDecodedRobotAudioFrame(_fileStream, audioChunkSize - 8),
									(audioChunkSize - 8) * 2, DisposeAfterUse::NO,
									Audio::FLAG_16BITS | Audio::FLAG_LITTLE_ENDIAN);
	} else {
		_fileStream->skip(audioChunkSize);
	}

	if (_curFrame == -1)
		_startTime = g_system->getMillis();

	_curFrame++;

	return _surface;
}

void RobotDecoder::close() {
	if (!_fileStream)
		return;

	delete _fileStream;
	_fileStream = 0;

	_surface->free();
	delete _surface;
	_surface = 0;

	if (_header.hasSound) {
		_mixer->stopHandle(_audioHandle);
		//delete _audioStream; _audioStream = 0;
	}

	reset();
}

void RobotDecoder::updateVolume() {
	if (g_system->getMixer()->isSoundHandleActive(_audioHandle))
		g_system->getMixer()->setChannelVolume(_audioHandle, getVolume());
}

void RobotDecoder::updateBalance() {
	if (g_system->getMixer()->isSoundHandleActive(_audioHandle))
		g_system->getMixer()->setChannelBalance(_audioHandle, getBalance());
}

#endif

} // End of namespace Sci
