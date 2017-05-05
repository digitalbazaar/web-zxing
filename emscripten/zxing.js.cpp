// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  Copyright 2010-2011 ZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <zxing/common/Counted.h>
#include <zxing/Binarizer.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/Result.h>
#include <zxing/ReaderException.h>
#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <exception>
#include <zxing/Exception.h>
#include <zxing/common/IllegalArgumentException.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>

#include <zxing/qrcode/QRCodeReader.h>
#include <zxing/multi/qrcode/QRCodeMultiReader.h>
#include <zxing/multi/ByQuadrantReader.h>
#include <zxing/multi/MultipleBarcodeReader.h>
#include <zxing/multi/GenericMultipleBarcodeReader.h>

#include <zxing/LuminanceSource.h>

#include <zxing/Binarizer.h>
#include <zxing/common/BitArray.h>
#include <zxing/common/BitMatrix.h>
#include <zxing/common/Array.h>

#include <emscripten.h>
#include <emscripten/bind.h>
using namespace std;
using namespace zxing;
using namespace zxing::qrcode;
using namespace zxing::multi;
using namespace emscripten;
struct Point2DF {
  float x;
  float y;
};

struct Quadrilateral {
  Point2DF north_east;
  Point2DF south_east;
  Point2DF south_west;
  Point2DF north_west;
};

struct ZXingResult {
  string data;
  Quadrilateral locus;
};

class ImageReaderSource : public zxing::LuminanceSource {
private:
  typedef LuminanceSource Super;

  const zxing::ArrayRef<char> image;

  char convertPixel(const char* pixel) const;

public:
  ImageReaderSource(zxing::ArrayRef<char> image, int width, int height);

  zxing::ArrayRef<char> getRow(int y, zxing::ArrayRef<char> row) const;
  zxing::ArrayRef<char> getMatrix() const;
};

ImageReaderSource::ImageReaderSource(ArrayRef<char> image_, int width, int height)
    : Super(width, height), image(image_) {}

zxing::ArrayRef<char> ImageReaderSource::getRow(int y, zxing::ArrayRef<char> row) const {
  const char* pixelRow = &image[0] + y * getWidth();
  if (!row) {
    row = zxing::ArrayRef<char>(getWidth());
  }
  for (int x = 0; x < getWidth(); x++) {
    row[x] = pixelRow[x];
  }
  return row;
}

zxing::ArrayRef<char> ImageReaderSource::getMatrix() const {
  return image;
  // zxing::ArrayRef<char> matrix(getWidth() * getHeight());
  // memcpy(&matrix[0], &image[0], getWidth() * getHeight());
  // return matrix;
}


class PassthroughBinarizer : public Binarizer {
private:
  ArrayRef<char> luminances;
public:
  PassthroughBinarizer(Ref<LuminanceSource> source);
  virtual ~PassthroughBinarizer();
    
  virtual Ref<BitArray> getBlackRow(int y, Ref<BitArray> row);
  virtual Ref<BitMatrix> getBlackMatrix();
  Ref<Binarizer> createBinarizer(Ref<LuminanceSource> source);
private:
  void initArrays(int luminanceSize);
};




using zxing::GlobalHistogramBinarizer;
using zxing::Binarizer;
using zxing::ArrayRef;
using zxing::Ref;
using zxing::BitArray;
using zxing::BitMatrix;

// VC++
using zxing::LuminanceSource;

namespace {
  const ArrayRef<char> EMPTY (0);
}

PassthroughBinarizer::PassthroughBinarizer(Ref<LuminanceSource> source) 
  : Binarizer(source), luminances(EMPTY) {}

PassthroughBinarizer::~PassthroughBinarizer() {}

void PassthroughBinarizer::initArrays(int luminanceSize) {
  if (luminances->size() < luminanceSize) {
    luminances = ArrayRef<char>(luminanceSize);
  }
}

Ref<BitArray> PassthroughBinarizer::getBlackRow(int y, Ref<BitArray> row) {
  // std::cerr << "gbr " << y << std::endl;
  LuminanceSource& source = *getLuminanceSource();
  int width = source.getWidth();
  if (row == NULL || static_cast<int>(row->getSize()) < width) {
    row = new BitArray(width);
  } else {
    row->clear();
  }

  initArrays(width);
  ArrayRef<char> localLuminances = source.getRow(y, luminances);
  for (int x = 0; x < width; x++) {
    if (luminances[x]) {
      row->set(x);
    }
  }
  return row;
}
 
Ref<BitMatrix> PassthroughBinarizer::getBlackMatrix() {
  LuminanceSource& source = *getLuminanceSource();
  int width = source.getWidth();
  int height = source.getHeight();
  Ref<BitMatrix> matrix(new BitMatrix(width, height));

  ArrayRef<char> localLuminances = source.getMatrix();
  for (int y = 0; y < height; y++) {
    int offset = y * width;
    for (int x = 0; x < width; x++) {
      if (localLuminances[offset + x]) {
        matrix->set(x, y);
      }
    }
  }
  
  return matrix;
}

Ref<Binarizer> PassthroughBinarizer::createBinarizer(Ref<LuminanceSource> source) {
  return Ref<Binarizer> (new PassthroughBinarizer(source));
}

vector<Ref<Result> > decode_qr_(Ref<BinaryBitmap> image, DecodeHints hints) {
  Ref<Reader> qrCodeReader(new QRCodeReader);
  return vector<Ref<Result> >(1, qrCodeReader->decode(image, hints));
}

vector<Ref<Result> > decode_qr_multi_(Ref<BinaryBitmap> image, DecodeHints hints) {
  Ref<MultipleBarcodeReader> qrCodeMultiReader(new QRCodeMultiReader);
  return qrCodeMultiReader->decodeMultiple(image, hints);
}

vector<Ref<Result> > decode_any_(Ref<BinaryBitmap> image, DecodeHints hints) {
  Ref<Reader> multiFormatReader(new MultiFormatReader);
  return vector<Ref<Result> >(1, multiFormatReader->decode(image, hints));
}

vector<Ref<Result> > decode_multi_(Ref<BinaryBitmap> image, DecodeHints hints) {
  MultiFormatReader delegate;
  GenericMultipleBarcodeReader genericReader(delegate);
  return genericReader.decodeMultiple(image, hints);
}

enum DECODE_MODE {
  QR,
  QR_MULTI,
  ANY,
  MULTI
};


extern "C" {

  vector<ZXingResult> * __decode(DECODE_MODE mode, unsigned char * data, int width, int height) {
    vector<ZXingResult> * ex_results = new vector<ZXingResult>();
    Ref<Binarizer> binarizer;
    Ref<BinaryBitmap> binary;
    vector<Ref<Result> > results;
    zxing::ArrayRef<char> image = zxing::ArrayRef<char>(width*height);
    Ref<LuminanceSource> source = Ref<LuminanceSource>(new ImageReaderSource(image, width, height));
    for(int i = 0; i < height*width; i++) {
      image[i] = data[i*4];
    }
    try {

      DecodeHints hints(DecodeHints::DEFAULT_HINT);

      binarizer = new HybridBinarizer(source);
      binary = new BinaryBitmap(binarizer);
      switch(mode) {
        case DECODE_MODE::QR:
          results = decode_qr_(binary, hints);
          break;
        case DECODE_MODE::QR_MULTI:
          results = decode_qr_multi_(binary, hints);
          break;
        case DECODE_MODE::ANY:
          results = decode_any_(binary, hints);
          break;
        case DECODE_MODE::MULTI:
          results = decode_multi_(binary, hints);
          break;
      }
      for (int i=0; i<results.size(); i++) {
        auto points = results[i]->getResultPoints();
        Quadrilateral locus = {
          {points[0]->getX(), points[0]->getY()},
          {points[1]->getX(), points[1]->getY()},
          {points[2]->getX(), points[2]->getY()},
          {points[3]->getX(), points[3]->getY()}
        };
        ZXingResult newResult = {results[i]->getText()->getText(),locus};
        ex_results->push_back(newResult);
      }
    } catch (const ReaderException& e) {
      // cout << e << endl;
      cout << "reader exception" << endl;
    } catch (const zxing::IllegalArgumentException& e) {
      // cout << e << endl;
      cout << "illegal arg" << endl;
    } catch (const zxing::Exception& e) {
      // cout << e << endl;
      cout << "general zxing error" << endl;
    } catch (const std::exception& e) {
      // cout << e << endl;
    }
    return ex_results;
  }

  vector<ZXingResult> * decode_qr(unsigned char * data, int width, int height) {
    return __decode(DECODE_MODE::QR, data, width, height);
  }

  vector<ZXingResult> * decode_qr_multi(unsigned char * data, int width, int height) {
    return __decode(DECODE_MODE::QR_MULTI, data, width, height);
  }

  vector<ZXingResult> * decode_any(unsigned char * data, int width, int height) {
    return __decode(DECODE_MODE::ANY, data, width, height);
  }

  vector<ZXingResult> * decode_multi(unsigned char * data, int width, int height) {
    return __decode(DECODE_MODE::MULTI, data, width, height);
  }
}

vector<ZXingResult> * vectorZXingResultFromIntPointer(uintptr_t ptr) {
    return reinterpret_cast<vector<ZXingResult > *>(ptr);
}

EMSCRIPTEN_BINDINGS(zxing) {
  value_object<Point2DF>("Point2DF")
    .field("x", &Point2DF::x)
    .field("y", &Point2DF::y)
    ;

  value_object<Quadrilateral>("Quadrilateral")
    .field("north_east", &Quadrilateral::north_east)
    .field("south_east", &Quadrilateral::south_east)
    .field("south_west", &Quadrilateral::south_west)
    .field("north_west", &Quadrilateral::north_west)
    ;
  value_object<ZXingResult>("ZXingResult")
    .field("data", &ZXingResult::data)
    .field("locus", &ZXingResult::locus)
    ;
  register_vector<ZXingResult>("VectorZXingResult").constructor(&vectorZXingResultFromIntPointer, allow_raw_pointers());
}