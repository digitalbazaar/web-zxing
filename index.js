function resample_single(canvas, width, height, resize_canvas) {
    var width_source = canvas.width;
    var height_source = canvas.height;
    width = Math.round(width);
    height = Math.round(height);

    var ratio_w = width_source / width;
    var ratio_h = height_source / height;
    var ratio_w_half = Math.ceil(ratio_w / 2);
    var ratio_h_half = Math.ceil(ratio_h / 2);

    var ctx = canvas.getContext("2d");
    var img = ctx.getImageData(0, 0, width_source, height_source);
    var img2 = ctx.createImageData(width, height);
    var data = img.data;
    var data2 = img2.data;

    for (var j = 0; j < height; j++) {
        for (var i = 0; i < width; i++) {
            var x2 = (i + j * width) * 4;
            var weight = 0;
            var weights = 0;
            var weights_alpha = 0;
            var gx_r = 0;
            var gx_g = 0;
            var gx_b = 0;
            var gx_a = 0;
            var center_y = (j + 0.5) * ratio_h;
            var yy_start = Math.floor(j * ratio_h);
            var yy_stop = Math.ceil((j + 1) * ratio_h);
            for (var yy = yy_start; yy < yy_stop; yy++) {
                var dy = Math.abs(center_y - (yy + 0.5)) / ratio_h_half;
                var center_x = (i + 0.5) * ratio_w;
                var w0 = dy * dy; //pre-calc part of w
                var xx_start = Math.floor(i * ratio_w);
                var xx_stop = Math.ceil((i + 1) * ratio_w);
                for (var xx = xx_start; xx < xx_stop; xx++) {
                    var dx = Math.abs(center_x - (xx + 0.5)) / ratio_w_half;
                    var w = Math.sqrt(w0 + dx * dx);
                    if (w >= 1) {
                        //pixel too far
                        continue;
                    }
                    //hermite filter
                    weight = 2 * w * w * w - 3 * w * w + 1;
                    var pos_x = 4 * (xx + yy * width_source);
                    //alpha
                    gx_a += weight * data[pos_x + 3];
                    weights_alpha += weight;
                    //colors
                    if (data[pos_x + 3] < 255)
                        weight = weight * data[pos_x + 3] / 250;
                    gx_r += weight * data[pos_x];
                    gx_g += weight * data[pos_x + 1];
                    gx_b += weight * data[pos_x + 2];
                    weights += weight;
                }
            }
            data2[x2] = gx_r / weights;
            data2[x2 + 1] = gx_g / weights;
            data2[x2 + 2] = gx_b / weights;
            data2[x2 + 3] = gx_a / weights_alpha;
        }
    }
    //clear and resize canvas
    if (resize_canvas === true) {
        canvas.width = width;
        canvas.height = height;
    } else {
        ctx.clearRect(0, 0, width_source, height_source);
    }

    //draw
    ctx.putImageData(img2, 0, 0);
}

function testZXing() {
	var img = new Image;
	img.src = 'emscripten/test/Qr-2.png';
	img.onload = function() {

		var width = Math.floor(this.width),
			height = Math.floor(this.height);

		var canvas = document.createElement('canvas');
		canvas.style.display = 'block';
		canvas.width = width;
		canvas.height = height;
		var ctx = canvas.getContext('2d');
		// ctx.rotate(Math.random()*0.1-0.05);
		ctx.drawImage(this, 0, 0, width, height);
		resample_single(canvas, 55, 55, true);
		width = canvas.width;
		height = canvas.height;
		var imageData = ctx.getImageData(0, 0, width, height);
		var idd = imageData.data;
		document.body.appendChild(canvas);

		// the crucial bit
		var len = idd.length;
		var image = ZXing._malloc(len);
		ZXing.HEAPU8.set(idd, image);

		var ptr = ZXing._decode_qr(image, width, height);
		// instantiate, read and teardown
		detected_codes = new ZXing.VectorZXingResult(ptr);
		for(i = 0; i < detected_codes.size(); i++) {
			console.log(detected_codes.get(i))
		}
		detected_codes.delete();
		// var image = window.ZXing._resize(width, height);
		console.time("decode QR");
		// for (var i=0, j=0; i<idd.length; i+=4, j++) {
		// 	ZXing.HEAPU8[image + j] = idd[i];
		// }
		var len = idd.length;
		var image = ZXing._malloc(len);
		ZXing.HEAPU8.set(idd, image);
		ptr = ZXing._decode_qr(image,width,height);
		detected_codes = new ZXing.VectorZXingResult(ptr);
		detected_codes.delete();
		console.timeEnd("decode QR");

	};
};

var script = document;
doneEvent = new Event('done');
script.addEventListener('done', function() {
    console.log(ZXing);
    testZXing();
})
ZXing = new ZXing();
