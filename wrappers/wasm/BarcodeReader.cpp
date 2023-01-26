/*
* Copyright 2016 Nu-book Inc.
*/
// SPDX-License-Identifier: Apache-2.0

#include "ReadBarcode.h"

#include <string>
#include <memory>
#include <stdexcept>
#include <emscripten/bind.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct ReadResult
{
	std::string format;
	std::string text;
	std::string error;
	ZXing::Position position;
};

std::vector<ReadResult> readBarcodesFromImageView(ZXing::ImageView iv, bool tryHarder, const std::string& format, const int maxSymbols)
{
	std::vector<ReadResult> results{};

	using namespace ZXing;
	try {
		DecodeHints hints;
		hints.setTryHarder(tryHarder);
		hints.setTryRotate(tryHarder);
		hints.setTryInvert(tryHarder);
		hints.setTryDownscale(tryHarder);
		hints.setFormats(BarcodeFormatsFromString(format));
		hints.setMaxNumberOfSymbols(maxSymbols);

		auto rawResults = ReadBarcodes(iv, hints);
		for(int i = 0; i < rawResults.size(); i++)
		{
			auto& result = rawResults[i];
			results.emplace_back(ReadResult{ToString(result.format()), result.text(), "", result.position()});
		}
	}
	catch (const std::exception& e) {
		results.emplace_back(ReadResult{"", "", e.what()});
	}
	catch (...) {
		results.emplace_back(ReadResult{"", "", "Unknown error"});
	}

	return results;
}

ReadResult readBarcodeFromImage(int bufferPtr, int bufferLength, bool tryHarder, std::string format)
{
	using namespace ZXing;

	int width, height, channels;
	std::unique_ptr<stbi_uc, void (*)(void*)> buffer(
		stbi_load_from_memory(reinterpret_cast<const unsigned char*>(bufferPtr), bufferLength, &width, &height, &channels, 4),
		stbi_image_free);
	if (buffer == nullptr) {
		return {"", "", "Error loading image"};
	}

	auto results = readBarcodesFromImageView({buffer.get(), width, height, ImageFormat::RGBX}, tryHarder, format, 1);
	if (results.size() > 0)
	{
		return results.front();
	}

	return {};
}

ReadResult readBarcodeFromPixmap(int bufferPtr, int imgWidth, int imgHeight, bool tryHarder, std::string format)
{
	using namespace ZXing;
	auto results = readBarcodesFromImageView({reinterpret_cast<uint8_t*>(bufferPtr), imgWidth, imgHeight, ImageFormat::RGBX}, tryHarder, format, 1);
	if (results.size() > 0)
	{
		return results.front();
	}

	return {};
}

std::vector<ReadResult> readBarcodesFromPixmap(int bufferPtr, int imgWidth, int imgHeight, bool tryHarder, std::string format)
{
	using namespace ZXing;
	return readBarcodesFromImageView({reinterpret_cast<uint8_t*>(bufferPtr), imgWidth, imgHeight, ImageFormat::RGBX}, tryHarder, format, 255);
}

EMSCRIPTEN_BINDINGS(BarcodeReader)
{
	using namespace emscripten;

	value_object<ReadResult>("ReadResult")
			.field("format", &ReadResult::format)
			.field("text", &ReadResult::text)
			.field("error", &ReadResult::error)
			.field("position", &ReadResult::position)
			;

	value_object<ZXing::PointI>("Point")
			.field("x", &ZXing::PointI::x)
			.field("y", &ZXing::PointI::y)
			;

	value_object<ZXing::Position>("Position")
			.field("topLeft", emscripten::index<0>())
			.field("topRight", emscripten::index<1>())
			.field("bottomRight", emscripten::index<2>())
			.field("bottomLeft", emscripten::index<3>())
			;

	function("readBarcodeFromImage", &readBarcodeFromImage);
	function("readBarcodeFromPixmap", &readBarcodeFromPixmap);
	function("readBarcodesFromPixmap", &readBarcodesFromPixmap);

	register_vector<ReadResult>("vector<ReadResult>");
}
