#pragma once

#define ColorRGB		"RGB"
#define ColorRGB2XYZ	"RGB2XYZ"
#define ColorXYZ		"XYZ"
#define ColorLab		"Lab"
#define ColorHSL		"HSL"

static double g_MatrixD65[3][3] = {{0.4124564, 0.3575761, 0.1804375}, 
							       {0.2126729, 0.7151522, 0.0721750},
							       {0.0193339, 0.1191920, 0.9503041}};

static double g_MatrixInverseD65[3][3] = {{3.2404542, -1.5371385, -0.4985314}, 
										  {-0.9692660, 1.8760108, 0.0415560}, 
									      {0.0556434, -0.2040259, 1.0572252}};

#define DEFAULT_BUFLEN		1024
#define DEFAULT_PORT		27015
#define FINISH_RECEIVE		"Receive completed!!"
#define START_COLOR_PATCH	"Start color patch"

#define CLOSE_SOCKET		"Close"
#define CONTINUE			"Continue"

#define TIME_OUT			2000

#define SAVE_IMAGE_PATH		"D:\\Image data\\TestImage_"