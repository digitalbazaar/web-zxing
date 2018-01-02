// wasm.config.js
module.exports = {
  emscripten_path: './../emsdk-portable',
  outputfile: './wasm/zxing.js',
  mode: 'build',
  exported_functions: [
    '_decode_qr',
    '_decode_qr_multi',
    '_decode_multi',
    '_decode_any',
  ],
  flags: [
    '-s WASM=1',
    '-std=c++11',
    '-s ASSERTIONS=1',
    '-O3',
    '-s RESERVED_FUNCTION_POINTERS=20',
    '--bind',
    '-s MODULARIZE=1',
    '-s EXPORT_NAME=\"\'ZXing\'\"',
    '-s DISABLE_EXCEPTION_CATCHING=0',
    '--pre-js \"prejs.js\"'
  ],
};
