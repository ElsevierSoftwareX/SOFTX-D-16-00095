
//************************************************************************************************************//
//***Sepand X-Ray Inspector (SXI1) Software. Designed by Alireza Alipour (a.alipouramlashi@gmail.com)       ***//
//***Image Processing filters for Optimizing X-ray Images containing:                                      ***//
//***Eliminating background noise, Noise reduction, Image contrast enhancement and Image pseudo coloring.  ***//
//***SXI ver 1.1 ; Date: 2016/11/14 , Copyright (c) 2014-2016, Alireza Alipour. All rights reserved.       ***//
//***                                                                                                      ***//
//***Code implemented in microsoft visual studio ultimate 2013 compiler in 64-bit (Win64).                 ***//
//***                                                                                                      ***//
//***Reference:                                                                                            ***//
//***[1] OpenCV (Open Source Computer Vision), http://opencv.org/                                          ***//
//***[2] GTK (GIMP Toolki), http://www.gtk.org/                                                            ***//
//                                                                                                         ***//
//************************************************************************************************************//              
#include <opencv2/opencv.hpp>
#include <gtk/gtk.h>

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

using namespace cv;
using namespace std;

//Matrices and vecrores
Mat src1, src2, Tafrigh, SPDENO, im_color, dst1,
DefreqNoise, dstt, thresh, dst, unsharp;

Mat alpha;
vector<Mat> rgb;

char *filename;
char *foldername;


//global variables
GdkPixbuf *img1buffer = NULL, *croppic = NULL, *img1buffer_resized = NULL;

gint width = 0, height = 0;
gint new_width = 0, new_height = 0;
gint orig_width = 0, orig_height = 0;

gint dest_x = 0, dest_y = 0,
dest_width = 0, dest_height = 0;

GtkWidget *win, *but3,*but13,*img1, *img2, *img3, *img4, *img5, *table,
*frame1, *frame2, *frame3, *frame4, *rotate_combo,*width_combo,
*height_combo, *colorize_combo, *threshold_combo, *fast_combo, *ref_combo;

GtkWidget* widget, *u_name, *pass, *window, *event_box, *fileMi, *newMi,
*openMi, *cropMi, *rotateMi,*rotateMi1, *rotateMi2, *rotateMi3, *rotateMi4,
*rotateMi5, *rotateMi6, *rotateMi7, *rotateMi8,*resizeMi, *imprMenu, *runMi, *quitMi, *sep ;


GdkColor color;
GtkBuilder *printui;
gpointer data;
GdkEvent *event;
//=======================================================================================================
//simulation function1 
//=======================================================================================================
void SXI1(GtkWidget *widget, gpointer data)
{
	if (img1buffer == NULL)
		return;
	/************************************/
	/*Load Raw X-ray images*/
	/************************************/
	GdkPixbuf *img1buffer_resized = gdk_pixbuf_scale_simple(img1buffer, width, height, GDK_INTERP_NEAREST);
	gdk_pixbuf_save(img1buffer_resized, "C:/Program Files (x86)/Sepand64bit/imagedata/Raw X-Ray Image.png", "png", NULL, NULL);

	Mat image = imread("C:/Program Files (x86)/Sepand64bit/imagedata/Raw X-Ray Image.png");
	cvtColor(image, src1, CV_RGB2GRAY);

	imwrite("C:/Program Files (x86)/Sepand64bit/imagedata/Raw X-Ray Image.png", src1);
	gtk_image_set_from_file(GTK_IMAGE(img1), "C:/Program Files (x86)/Sepand64bit/imagedata/Raw X-Ray Image.png");

	/************************************/
	/*Eliminating background noise*/
	/************************************/
	threshold(src1, thresh, 170, 255, THRESH_BINARY_INV);
	erode(thresh, thresh, cv::Mat(), cv::Point(-1, -1));
	GaussianBlur(thresh, thresh, cv::Size(0, 0), 1);

	vector< Vec4i > hierarchy;
	int largest_contour_index = 0;
	int largest_area = 0;

	vector< vector <Point> > contours1;
	Mat alpha(src1.size(), CV_8UC1, Scalar(0));
	normalize(alpha, alpha, 0, 256, NORM_MINMAX, CV_8UC3);
	findContours(thresh, contours1, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); // Find the contours in the image
	for (int i = 0; i< contours1.size(); i++) // iterate through each contour.
	{
		double a = contourArea(contours1[i], false);  //  Find the area of contour
		if (a>largest_area){
			largest_area = a;
			largest_contour_index = i;                //Store the index of largest contour
		}
	}
	drawContours(alpha, contours1, largest_contour_index, Scalar(255), CV_FILLED, 8, hierarchy);

	Mat rgba[4] = { src1, src1, src1, alpha };
	merge(rgba, 4, Tafrigh);

	imwrite("C:/Program Files (x86)/Sepand64bit/imagedata/Subtracted X-Ray Image.png", Tafrigh);
	gtk_image_set_from_file(GTK_IMAGE(img2), "C:/Program Files (x86)/Sepand64bit/imagedata/Subtracted X-Ray Image.png");
	/************************************/
	/*Stripe Noises reduction*/
	/************************************/
	cvtColor(Tafrigh, src2, CV_RGB2GRAY);
	vector<double> moyenne;
	for (int i = 0; i < src2.rows; i++)
	{
		double s = 0;
		// Caluclate mean for row i
		for (int j = 0; j<src2.cols; j++)
			s += src1.at<uchar>(i, j);
		// Store result in vector moyenne
		moyenne.push_back(s / src2.cols);
	}
	//// Energy for row i equal to a weighted mean of row in [i-nbInf,i+nbSup]
	int nbInf = 128, nbSup = 128;
	for (int i = 0; i < src2.rows; i++)
	{
		double s = 0, p = 0;
		// weighted mean (border effect process with max and min method
		for (int j = max(0, i - nbInf); j <= min(src2.rows - 1, i + nbSup); j++)
		{
			s += moyenne[j] * 1. / (1 + abs(i - j));
			p += 1. / (1 + abs(i - j));
		}
		// Weighted mean
		s /= p;
		// process pixel in row i : mean of row i equal to s 
		for (int j = 0; j<src2.cols; j++)
			src2.at<uchar>(i, j) = saturate_cast<uchar>((src2.at<uchar>(i, j) - moyenne[i]) + s);
	}

	Mat rgbb[4] = { src2, src2, src2, alpha };
	merge(rgbb, 4, DefreqNoise);
	/************************************/
	/*Salt-Pepper noises reduction*/
	/************************************/
	fastNlMeansDenoising(DefreqNoise, DefreqNoise, 5, 5, 5 * 2);
	fastNlMeansDenoisingColored(DefreqNoise, dst, 5, 5, 5 * 2, 5 * 3);

	split(dst, rgb);
	Mat rgbc[4] = { rgb[0], rgb[1], rgb[2], alpha };
	merge(rgbc, 4, SPDENO);

	/************************************/
	/*Image contrast enhancement*/
	/************************************/
	GaussianBlur(SPDENO, unsharp, cv::Size(0, 0), 4);
	addWeighted(SPDENO, 2.17, unsharp, -1, 0, unsharp);

	imwrite("C:/Program Files (x86)/Sepand64bit/imagedata/Denoised X-Ray Image.png", unsharp);
	gtk_image_set_from_file(GTK_IMAGE(img3), "C:/Program Files (x86)/Sepand64bit/imagedata/Denoised X-Ray Image.png");

	/************************************/
	/*Image pseudo coloring*/
	/************************************/
	cvtColor(unsharp, unsharp, CV_RGB2GRAY);

	applyColorMap(unsharp, dstt, 11);
	split(dstt, rgb);
	Mat rgbe[4] = { rgb[0], rgb[1], rgb[2], alpha };
	merge(rgbe, 4, im_color);

	imwrite("C:/Program Files (x86)/Sepand64bit/imagedata/Pseudo Colored X-Ray Image.png", im_color);
	gtk_image_set_from_file(GTK_IMAGE(img4), "C:/Program Files (x86)/Sepand64bit/imagedata/Pseudo Colored X-Ray Image.png");
}
//=======================================================================================================
//simulation function2
//=======================================================================================================
void SXI2(GtkWidget *widget, const gchar *mode)
{

	if (!g_strcmp0(mode, "threshold"))
	{
		if (gtk_image_get_pixbuf(GTK_IMAGE(img2)) == NULL)
			return;

		int threshold_value = atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(threshold_combo)));

		threshold(src1, thresh, threshold_value, 255, THRESH_BINARY_INV);
		erode(thresh, thresh, cv::Mat(), cv::Point(-1, -1));
		GaussianBlur(thresh, thresh, cv::Size(0, 0), 1);

		vector< Vec4i > hierarchy;
		int largest_contour_index = 0;
		int largest_area = 0;

		vector< vector <Point> > contours1;
		Mat alpha(src1.size(), CV_8UC1, Scalar(0));
		normalize(alpha, alpha, 0, 256, NORM_MINMAX, CV_8UC3);
		findContours(thresh, contours1, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); // Find the contours in the image
		for (int i = 0; i< contours1.size(); i++) // iterate through each contour.
		{
			double a = contourArea(contours1[i], false);  //  Find the area of contour
			if (a>largest_area){
				largest_area = a;
				largest_contour_index = i;                //Store the index of largest contour
			}
		}
		drawContours(alpha, contours1, largest_contour_index, Scalar(255), CV_FILLED, 8, hierarchy);

		Mat rgba[4] = { src1, src1, src1, alpha };
		merge(rgba, 4, Tafrigh);

		imwrite("C:/Program Files (x86)/Sepand64bit/imagedata/Subtracted X-Ray Image.png", Tafrigh);
		gtk_image_set_from_file(GTK_IMAGE(img2), "C:/Program Files (x86)/Sepand64bit/imagedata/Subtracted X-Ray Image.png");

	}
	else if (!g_strcmp0(mode, "fast"))
	{
		if (gtk_image_get_pixbuf(GTK_IMAGE(img3)) == NULL)
			return;

		int fast_value = atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(fast_combo)));

		vector< Vec4i > hierarchy;
		int largest_contour_index = 0;
		int largest_area = 0;

		vector< vector <Point> > contours1;
		Mat alpha(src1.size(), CV_8UC1, Scalar(0));
		normalize(alpha, alpha, 0, 256, NORM_MINMAX, CV_8UC3);
		findContours(thresh, contours1, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); // Find the contours in the image
		for (int i = 0; i< contours1.size(); i++) // iterate through each contour.
		{
			double a = contourArea(contours1[i], false);  //  Find the area of contour
			if (a>largest_area){
				largest_area = a;
				largest_contour_index = i;                //Store the index of largest contour
			}
		}
		drawContours(alpha, contours1, largest_contour_index, Scalar(255), CV_FILLED, 8, hierarchy);
		/************************************/
		/*Stripe Noises reduction*/
		/************************************/
		cvtColor(Tafrigh, src2, CV_RGB2GRAY);
		vector<double> moyenne;
		for (int i = 0; i < src2.rows; i++)
		{
			double s = 0;
			// Caluclate mean for row i
			for (int j = 0; j<src2.cols; j++)
				s += src1.at<uchar>(i, j);
			// Store result in vector moyenne
			moyenne.push_back(s / src2.cols);
		}
		//// Energy for row i equal to a weighted mean of row in [i-nbInf,i+nbSup]
		int nbInf = 128, nbSup = 128;
		for (int i = 0; i < src2.rows; i++)
		{
			double s = 0, p = 0;
			// weighted mean (border effect process with max and min method
			for (int j = max(0, i - nbInf); j <= min(src2.rows - 1, i + nbSup); j++)
			{
				s += moyenne[j] * 1. / (1 + abs(i - j));
				p += 1. / (1 + abs(i - j));
			}
			// Weighted mean
			s /= p;
			// process pixel in row i : mean of row i equal to s 
			for (int j = 0; j<src2.cols; j++)
				src2.at<uchar>(i, j) = saturate_cast<uchar>((src2.at<uchar>(i, j) - moyenne[i]) + s);
		}

		Mat rgbb[4] = { src2, src2, src2, alpha };
		merge(rgbb, 4, DefreqNoise);
		/************************************/
		/*Salt-Pepper noises reduction*/
		/************************************/
		fastNlMeansDenoising(DefreqNoise, DefreqNoise, fast_value, fast_value, fast_value * 2);
		fastNlMeansDenoisingColored(DefreqNoise, dst, fast_value, fast_value, fast_value * 2, fast_value * 3);

		vector<Mat> rgb;
		split(dst, rgb);
		Mat rgbc[4] = { rgb[0], rgb[1], rgb[2], alpha };
		merge(rgbc, 4, SPDENO);

		/************************************/
		/*Image contrast enhancement*/
		/************************************/
		GaussianBlur(SPDENO, unsharp, cv::Size(0, 0), 4);
		addWeighted(SPDENO, 2.17, unsharp, -1, 0, unsharp);

		imwrite("C:/Program Files (x86)/Sepand64bit/imagedata/Denoised X-Ray Image.png", unsharp);
		gtk_image_set_from_file(GTK_IMAGE(img3), "C:/Program Files (x86)/Sepand64bit/imagedata/Denoised X-Ray Image.png");

		/************************************/
		/*Image pseudo coloring*/
		/************************************/
		cvtColor(unsharp, unsharp, CV_RGB2GRAY);

		applyColorMap(unsharp, dstt, 11);

		split(dstt, rgb);
		Mat rgbe[4] = { rgb[0], rgb[1], rgb[2], alpha };
		merge(rgbe, 4, im_color);

		imwrite("C:/Program Files (x86)/Sepand64bit/imagedata/Pseudo Colored X-Ray Image.png", im_color);
		gtk_image_set_from_file(GTK_IMAGE(img4), "C:/Program Files (x86)/Sepand64bit/imagedata/Pseudo Colored X-Ray Image.png");

	}

}
//=======================================================================================================
//simulation function3
//=======================================================================================================
void SXI3(GtkWidget *widget, const gchar *mode)
{

	if (!g_strcmp0(mode, "fast"))
	{
		if (gtk_image_get_pixbuf(GTK_IMAGE(img3)) == NULL)
			return;

		int fast_value = atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(fast_combo)));

		vector< Vec4i > hierarchy;
		int largest_contour_index = 0;
		int largest_area = 0;

		vector< vector <Point> > contours1;
		Mat alpha(src1.size(), CV_8UC1, Scalar(0));
		normalize(alpha, alpha, 0, 256, NORM_MINMAX, CV_8UC3);
		findContours(thresh, contours1, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); // Find the contours in the image
		for (int i = 0; i< contours1.size(); i++) // iterate through each contour.
		{
			double a = contourArea(contours1[i], false);  //  Find the area of contour
			if (a>largest_area){
				largest_area = a;
				largest_contour_index = i;                //Store the index of largest contour
			}
		}
		drawContours(alpha, contours1, largest_contour_index, Scalar(255), CV_FILLED, 8, hierarchy);
		/************************************/
		/*Stripe Noises reduction*/
		/************************************/
		cvtColor(Tafrigh, src2, CV_RGB2GRAY);
		vector<double> moyenne;
		for (int i = 0; i < src2.rows; i++)
		{
			double s = 0;
			// Caluclate mean for row i
			for (int j = 0; j<src2.cols; j++)
				s += src1.at<uchar>(i, j);
			// Store result in vector moyenne
			moyenne.push_back(s / src2.cols);
		}
		//// Energy for row i equal to a weighted mean of row in [i-nbInf,i+nbSup]
		int nbInf = 128, nbSup = 128;
		for (int i = 0; i < src2.rows; i++)
		{
			double s = 0, p = 0;
			// weighted mean (border effect process with max and min method
			for (int j = max(0, i - nbInf); j <= min(src2.rows - 1, i + nbSup); j++)
			{
				s += moyenne[j] * 1. / (1 + abs(i - j));
				p += 1. / (1 + abs(i - j));
			}
			// Weighted mean
			s /= p;
			// process pixel in row i : mean of row i equal to s 
			for (int j = 0; j<src2.cols; j++)
				src2.at<uchar>(i, j) = saturate_cast<uchar>((src2.at<uchar>(i, j) - moyenne[i]) + s);
		}

		Mat rgbb[4] = { src2, src2, src2, alpha };
		merge(rgbb, 4, DefreqNoise);
		/************************************/
		/*Salt-Pepper noises reduction*/
		/************************************/
		fastNlMeansDenoising(DefreqNoise, DefreqNoise, fast_value, fast_value, fast_value * 2);
		fastNlMeansDenoisingColored(DefreqNoise, dst, fast_value, fast_value, fast_value * 2, fast_value * 3);

		vector<Mat> rgb;
		split(dst, rgb);
		Mat rgbc[4] = { rgb[0], rgb[1], rgb[2], alpha };
		merge(rgbc, 4, SPDENO);

		///************************************/
		///*Image contrast enhancement*/
		///************************************/
		GaussianBlur(SPDENO, unsharp, cv::Size(0, 0), 4);
		addWeighted(SPDENO, 2.17, unsharp, -1, 0, unsharp);

		imwrite("C:/Program Files (x86)/Sepand64bit/imagedata/Denoised X-Ray Image.png", unsharp);
		gtk_image_set_from_file(GTK_IMAGE(img3), "C:/Program Files (x86)/Sepand64bit/imagedata/Denoised X-Ray Image.png");

	}
	else if (!g_strcmp0(mode, "colorized"))
	{

		if (gtk_image_get_pixbuf(GTK_IMAGE(img4)) == NULL)
			return;

		static const char *color_names[] = { "AUTUMN", "BONE", "JET", "WINTER",
			"RAINBOW", "OCEAN", "SUMMER", "SPRING",
			"COOL", "HSV", "PINK", "HOT", "PARULA" };
		int type_color = -1;
		gchar * selected_color_name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(colorize_combo));
		for (int i = 0; i < G_N_ELEMENTS(color_names); i++)
			if (g_strcmp0(selected_color_name, color_names[i]) == 0)
				type_color = i;

		// gtk_combo_box_text_get_active_text return a string that must be freed as stated in the doc
		g_free(selected_color_name);


		vector< Vec4i > hierarchy;
		int largest_contour_index = 0;
		int largest_area = 0;

		vector< vector <Point> > contours1;
		Mat alpha(src1.size(), CV_8UC1, Scalar(0));
		normalize(alpha, alpha, 0, 256, NORM_MINMAX, CV_8UC3);
		findContours(thresh, contours1, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); // Find the contours in the image
		for (int i = 0; i< contours1.size(); i++) // iterate through each contour.
		{
			double a = contourArea(contours1[i], false);  //  Find the area of contour
			if (a>largest_area){
				largest_area = a;
				largest_contour_index = i;                //Store the index of largest contour
			}
		}
		drawContours(alpha, contours1, largest_contour_index, Scalar(255), CV_FILLED, 8, hierarchy);

		applyColorMap(unsharp, dstt, type_color);

		split(dstt, rgb);
		Mat rgbe[4] = { rgb[0], rgb[1], rgb[2], alpha };
		merge(rgbe, 4, im_color);

		imwrite("C:/Program Files (x86)/Sepand64bit/imagedata/Pseudo Colored X-Ray Image.png", im_color);
		gtk_image_set_from_file(GTK_IMAGE(img4), "C:/Program Files (x86)/Sepand64bit/imagedata/Pseudo Colored X-Ray Image.png");

	}

}
//==================================================================================================================
//browse image 
//==================================================================================================================
void browse(GtkWidget *widget, gpointer data)
{

	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Open your raw X-ray image from a file",
		GTK_WINDOW(win),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}

	gtk_widget_destroy(dialog);

	if (filename != NULL)
	{
		img1buffer = gdk_pixbuf_new_from_file(filename, NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(img1), img1buffer);
		width = gdk_pixbuf_get_width(img1buffer);
		height = gdk_pixbuf_get_height(img1buffer);
	}
}
//=======================================================================================================
//save button
//=======================================================================================================
void save(GtkWidget *widget, GtkToolButton *toolbutton, gpointer data)
{
	if (gtk_image_get_pixbuf(GTK_IMAGE(img2)) == NULL)
		return;

	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Select Folder For Saving",
		GTK_WINDOW(win),
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *nr_dir_mv, *ebn_dir_mv, *orig_dir_mv, *color_dir_mv;
		GFile *nr_file, *ebn_file, *orig_file, *color_file, *nr_file_mv, *ebn_file_mv, *orig_file_mv, *color_file_mv;

		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

		nr_file = g_file_new_for_path("C:/Program Files (x86)/Sepand64bit/imagedata/Denoised X-Ray Image.png");
		ebn_file = g_file_new_for_path("C:/Program Files (x86)/Sepand64bit/imagedata/Subtracted X-Ray Image.png");
		orig_file = g_file_new_for_path("C:/Program Files (x86)/Sepand64bit/imagedata/Raw X-Ray Image.png");
		color_file = g_file_new_for_path("C:/Program Files (x86)/Sepand64bit/imagedata/Pseudo Colored X-Ray Image.png");

		foldername = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		g_print(foldername);

		nr_dir_mv = g_build_filename(foldername, "Denoised X-Ray Image.png", NULL);
		ebn_dir_mv = g_build_filename(foldername, "Subtracted X-Ray Image.png", NULL);
		orig_dir_mv = g_build_filename(foldername, "Raw X-Ray Image.png", NULL);
		color_dir_mv = g_build_filename(foldername, "Pseudo Colored X-Ray Image.png", NULL);

		nr_file_mv = g_file_new_for_path(nr_dir_mv);
		ebn_file_mv = g_file_new_for_path(ebn_dir_mv);
		orig_file_mv = g_file_new_for_path(orig_dir_mv);
		color_file_mv = g_file_new_for_path(color_dir_mv);

		g_file_move(nr_file, nr_file_mv, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
		g_file_move(ebn_file, ebn_file_mv, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
		g_file_move(orig_file, orig_file_mv, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
		g_file_move(color_file, color_file_mv, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);

		g_free(foldername);
}
gtk_widget_destroy(dialog);

}
//=======================================================================================================
//Helper function for setting image based on zoom level
//=======================================================================================================
void set_img_zoom()
{
	if (img1buffer == NULL)
		return;

	GdkPixbuf *img1buffer_resized;
	img1buffer_resized = gdk_pixbuf_scale_simple(img1buffer, width, height, GDK_INTERP_NEAREST);

	gtk_image_set_from_pixbuf(GTK_IMAGE(img1), img1buffer_resized);

	//set crop area to zero
	dest_width = 0;
	dest_height = 0;

}
//=======================================================================================================
//rotate 
//=======================================================================================================
void rotation(GtkWidget *widget, int kkk)
{
	gtk_combo_box_set_active(GTK_COMBO_BOX(rotate_combo), kkk);

	if (img1buffer == NULL)
		return;

	gint tmp;
	gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(rotate_combo));

	switch (index)
	{
	case 0:
		img1buffer = gdk_pixbuf_rotate_simple(img1buffer, GDK_PIXBUF_ROTATE_CLOCKWISE);
		tmp = width;
		width = height;
		height = tmp;
		break;
	case 1:
		img1buffer = gdk_pixbuf_rotate_simple(img1buffer, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
		tmp = width;
		width = height;
		height = tmp;
		break;
	case 2:
		img1buffer = gdk_pixbuf_rotate_simple(img1buffer, GDK_PIXBUF_ROTATE_CLOCKWISE);
		img1buffer = gdk_pixbuf_rotate_simple(img1buffer, GDK_PIXBUF_ROTATE_CLOCKWISE);
		break;
	case 3:
		img1buffer = gdk_pixbuf_rotate_simple(img1buffer, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
		img1buffer = gdk_pixbuf_rotate_simple(img1buffer, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
		break;
	default:
		break;
	}

	set_img_zoom();
}
//=======================================================================================================
//image selection part
//=======================================================================================================
gboolean mouse_press_callback(GtkWidget *event_box,GdkEventButton *event, gpointer data, GtkWidget *widget, cairo_t *cr)
{
	if (img1buffer == NULL)
		return TRUE;

	static gint press_x = 0, press_y = 0, rel_x = 0, rel_y = 0;

	GtkAllocation ebox;
	gint img1_x_offset = 0, img1_y_offset = 0;
	gtk_widget_get_allocation(event_box, &ebox);
	img1_x_offset = (ebox.width - width) / 2;
	img1_y_offset = (ebox.height - height) / 2;

	if (event->type == GDK_BUTTON_PRESS)
	{
		press_x = event->x - img1_x_offset;
		press_y = event->y - img1_y_offset;
	}
	else if (event->type == GDK_BUTTON_RELEASE)
	{
		rel_x = event->x - img1_x_offset;
		rel_y = event->y - img1_y_offset;

		dest_x = rel_x < press_x ? rel_x : press_x;
		dest_y = rel_y < press_y ? rel_y : press_y;
		dest_width = abs(rel_x - press_x);
		dest_height = abs(rel_y - press_y);

		// mark user selection in image
		GdkPixbuf *img1buffer_resized = gdk_pixbuf_scale_simple(img1buffer, width, height, GDK_INTERP_NEAREST);

		croppic = gdk_pixbuf_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/crop_bg.png", NULL);

		gdk_pixbuf_composite(croppic, img1buffer_resized, dest_x, dest_y, dest_width, dest_height, 0, 0, 1, 1, GDK_INTERP_NEAREST, 170);

		gtk_image_set_from_pixbuf(GTK_IMAGE(img1), img1buffer_resized);
	}

	return TRUE;
}
//=======================================================================================================
//crop part
//=======================================================================================================
void crop(GtkWidget *widget, gpointer user_data)
{
	if (img1buffer == NULL)
		return;
	if (dest_width < 5 || dest_height < 5)
		return;

	GdkPixbuf *img1buffer_resized = gdk_pixbuf_scale_simple(img1buffer, width, height, GDK_INTERP_NEAREST);
	img1buffer = gdk_pixbuf_new_subpixbuf(img1buffer_resized, dest_x, dest_y, dest_width, dest_height);
	width = gdk_pixbuf_get_width(img1buffer);
	height = gdk_pixbuf_get_height(img1buffer);
	gtk_image_set_from_pixbuf(GTK_IMAGE(img1), img1buffer);

}
//=======================================================================================================
//Helper function for creating frame with bold label
//=======================================================================================================
GtkWidget *bold_frame(const gchar * label)
{
	gchar *label_bold = g_strdup_printf("<b>%s</b>", label);
	GtkWidget *gtklabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(gtklabel), label_bold);
	GtkWidget *frame = gtk_frame_new(NULL);
	gtk_frame_set_label_widget(GTK_FRAME(frame), gtklabel);
	g_free(label_bold);
	return frame;
}
//=======================================================================================================
//Helper function for creating button with bold label
//=======================================================================================================
GtkWidget *bold_button(const gchar *label)
{
	gchar *label_bold = g_strdup_printf("<b>%s</b>", label);
	GtkWidget *button = gtk_button_new_with_label("");
	GList *list = gtk_container_get_children(GTK_CONTAINER(button));
	gtk_label_set_markup(GTK_LABEL(list->data), label_bold);
	g_free(label_bold);
	return button;
}
//=======================================================================================================
//Helper function for creating button with bold label and icon
//=======================================================================================================
GtkWidget *bold_img_button(const gchar * label, const gchar *img)
{
	gchar *label_bold = g_strdup_printf("<b>%s</b>", label);
	GtkWidget *button = gtk_button_new();
	GtkWidget *gtklabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(gtklabel), label_bold);
	GtkWidget *image = gtk_image_new_from_file(img);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtklabel, TRUE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(button), hbox);
	g_free(label_bold);
	return button;
}
//================================================================================================================
//reference click
//================================================================================================================
static void refsxi(GtkWindow *win)
{
	const char *authors[] = { "Alireza Alipour (a.alipouramlashi@gmail.com<a.alipouramlashi@gmail.com>)", NULL };
	gchar* comments = { "Sepand X-Ray Inspector (SXI1) is a graphic user interface (GUI) of the control       \n"
		"and image visualization software. The SXI1 is a nonintrusive vehicles and luggage  \n"
		"inspection software for application of image processing techniques to inspection,  \n"
		"surveillance and ensuring safety in high security areas such as airport, railway    \n"
		"station, custom entrances, border crossing points, sea harbors and access points \n"
		"in military bases. The SXI1 is designed by using OpenCV and GTK+ libraries.         "

	};

	gchar* copyright = { "Copyright (c) 2014-2016, Alireza Alipour. All rights reserved." };
	const gchar *license =
		"GNU GENERAL PUBLIC LICENSE\n"
		"Version 2, June 1991\n\n"

		"Copyright (C) 1989, 1991 Free Software Foundation, Inc.,\n"
		"51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n"
		"Everyone is permitted to copy and distribute verbatim copies\n"
		"of this license document, but changing it is not allowed.\n\n"

		"Preamble\n\n"

		"The licenses for most software are designed to take away your\n"
		"freedom to share and change it.By contrast, the GNU General Public\n"
		"License is intended to guarantee your freedom to share and change free\n"
		"software--to make sure the software is free for all its users.This\n"
		"General Public License applies to most of the Free Software\n"
		"Foundation's software and to any other program whose authors commit to\n"
		"using it. (Some other Free Software Foundation software is covered by\n"
		"the GNU Lesser General Public License instead.)  You can apply it to\n"
		"your programs, too.\n\n"

		"When we speak of free software, we are referring to freedom, not\n"
		"price.Our General Public Licenses are designed to make sure that you\n"
		"have the freedom to distribute copies of free software (and charge for\n"
		"this service if you wish), that you receive source code or can get it\n"
		"if you want it, that you can change the software or use pieces of it\n"
		"in new free programs; and that you know you can do these things.\n\n"

		"To protect your rights, we need to make restrictions that forbid\n"
		"anyone to deny you these rights or to ask you to surrender the rights.\n"
		"These restrictions translate to certain responsibilities for you if you\n"
		"distribute copies of the software, or if you modify it.\n\n"

		"For example, if you distribute copies of such a program, whether\n"
		"gratis or for a fee, you must give the recipients all the rights that\n"
		"you have.You must make sure that they, too, receive or can get the\n"
		"source code.And you must show them these terms so they know their\n"
		"rights.\n\n"

		"We protect your rights with two steps: (1) copyright the software, and\n"
		"(2) offer you this license which gives you legal permission to copy,\n"
		"distribute and / or modify the software.\n\n"

		"Also, for each author's protection and ours, we want to make certain\n"
		"that everyone understands that there is no warranty for this free\n"
		"software.If the software is modified by someone else and passed on, we\n"
		"want its recipients to know that what they have is not the original, so\n"
		"that any problems introduced by others will not reflect on the original\n"
		"authors' reputations.\n\n"

		"Finally, any free program is threatened constantly by software\n"
		"patents.We wish to avoid the danger that redistributors of a free\n"
		"program will individually obtain patent licenses, in effect making the\n"
		"program proprietary.To prevent this, we have made it clear that any\n"
		"patent must be licensed for everyone's free use or not licensed at all.\n\n"

		"The precise terms and conditions for copying, distribution and\n\n"
		"modification follow.\n\n"

		"GNU GENERAL PUBLIC LICENSE\n"
		"TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n\n"

		"0. This License applies to any program or other work which contains\n"
		"a notice placed by the copyright holder saying it may be distributed\n"
		"under the terms of this General Public License.The ""Program"", below,\n"
		"refers to any such program or work, and a ""work based on the Program"" \n"
		"means either the Program or any derivative work under copyright law :\n"
		"that is to say, a work containing the Program or a portion of it,\n"
		"either verbatim or with modifications and / or translated into another\n"
		"language.  (Hereinafter, translation is included without limitation in\n"
		"the term ""modification"".)  Each licensee is addressed as ""you"".\n\n"

		"Activities other than copying, distribution and modification are not\n"
		"covered by this License; they are outside its scope.The act of\n"
		"running the Program is not restricted, and the output from the Program\n"
		"is covered only if its contents constitute a work based on the\n"
		"Program(independent of having been made by running the Program).\n"
		"Whether that is true depends on what the Program does.\n\n"

		"1. You may copy and distribute verbatim copies of the Program's\n"
		"source code as you receive it, in any medium, provided that you\n"
		"conspicuously and appropriately publish on each copy an appropriate\n"
		"copyright notice and disclaimer of warranty; keep intact all the\n"
		"notices that refer to this License and to the absence of any warranty;\n"
		"and give any other recipients of the Program a copy of this License\n"
		"along with the Program.\n\n"

		"You may charge a fee for the physical act of transferring a copy, and\n"
		"you may at your option offer warranty protection in exchange for a fee.\n\n"

		"2. You may modify your copy or copies of the Program or any portion\n"
		"of it, thus forming a work based on the Program, and copy and\n"
		"distribute such modifications or work under the terms of Section 1\n"
		"above, provided that you also meet all of these conditions:\n\n"

		"a) You must cause the modified files to carry prominent notices\n"
		"stating that you changed the files and the date of any change.\n\n"

		"b) You must cause any work that you distribute or publish, that in\n"
		"whole or in part contains or is derived from the Program or any\n"
		"part thereof, to be licensed as a whole at no charge to all third\n"
		"parties under the terms of this License.\n\n"

		"c) If the modified program normally reads commands interactively\n"
		"when run, you must cause it, when started running for such\n"
		"interactive use in the most ordinary way, to print or display an\n"
		"announcement including an appropriate copyright notice and a\n"
		"notice that there is no warranty(or else, saying that you provide\n"
		"a warranty) and that users may redistribute the program under\n"
		"these conditions, and telling the user how to view a copy of this\n"
		"License.  (Exception: if the Program itself is interactive but\n"
		"does not normally print such an announcement, your work based on\n"
		"the Program is not required to print an announcement.)\n\n"

		"These requirements apply to the modified work as a whole.If\n"
		"identifiable sections of that work are not derived from the Program,\n"
		"and can be reasonably considered independent and separate works in\n"
		"themselves, then this License, and its terms, do not apply to those\n"
		"sections when you distribute them as separate works.But when you\n"
		"distribute the same sections as part of a whole which is a work based\n"
		"on the Program, the distribution of the whole must be on the terms of\n"
		"this License, whose permissions for other licensees extend to the\n"
		"entire whole, and thus to each and every part regardless of who wrote it.\n\n"

		"Thus, it is not the intent of this section to claim rights or contest\n"
		"your rights to work written entirely by you; rather, the intent is to\n"
		"exercise the right to control the distribution of derivative or\n"
		"collective works based on the Program.\n\n"

		"In addition, mere aggregation of another work not based on the Program\n"
		"with the Program(or with a work based on the Program) on a volume of\n"
		"a storage or distribution medium does not bring the other work under\n"
		"the scope of this License.\n"

		"3. You may copy and distribute the Program (or a work based on it,\n"
		"under Section 2) in object code or executable form under the terms of\n"
		"Sections 1 and 2 above provided that you also do one of the following:\n\n"

		"a) Accompany it with the complete corresponding machine-readable\n"
		"source code, which must be distributed under the terms of Sections\n"
		"1 and 2 above on a medium customarily used for software interchange; or,\n\n"

		"b) Accompany it with a written offer, valid for at least three\n"
		"years, to give any third party, for a charge no more than your\n"
		"cost of physically performing source distribution, a complete\n"
		"machine - readable copy of the corresponding source code, to be\n"
		"distributed under the terms of Sections 1 and 2 above on a medium\n"
		"customarily used for software interchange; or,\n\n"

		"c) Accompany it with the information you received as to the offer\n"
		"to distribute corresponding source code.  (This alternative is\n"
		"allowed only for noncommercial distribution and only if you\n"
		"received the program in object code or executable form with such\n"
		"an offer, in accord with Subsection b above.)\n\n"

		"The source code for a work means the preferred form of the work for\n"
		"making modifications to it.For an executable work, complete source\n"
		"code means all the source code for all modules it contains, plus any\n"
		"associated interface definition files, plus the scripts used to\n"
		"control compilation and installation of the executable.However, as a\n"
		"special exception, the source code distributed need not include\n"
		"anything that is normally distributed(in either source or binary\n"
		"form) with the major components(compiler, kernel, and so on) of the\n"
		"operating system on which the executable runs, unless that component\n"
		"itself accompanies the executable.\n\n"

		"If distribution of executable or object code is made by offering\n"
		"access to copy from a designated place, then offering equivalent\n"
		"access to copy the source code from the same place counts as\n"
		"distribution of the source code, even though third parties are not\n"
		"compelled to copy the source along with the object code.\n\n"

		"4. You may not copy, modify, sublicense, or distribute the Program\n"
		"except as expressly provided under this License.Any attempt\n"
		"otherwise to copy, modify, sublicense or distribute the Program is\n"
		"void, and will automatically terminate your rights under this License.\n"
		"However, parties who have received copies, or rights, from you under\n"
		"this License will not have their licenses terminated so long as such\n"
		"parties remain in full compliance.\n\n"

		"5. You are not required to accept this License, since you have not\n"
		"signed it.However, nothing else grants you permission to modify or\n"
		"distribute the Program or its derivative works.These actions are\n"
		"prohibited by law if you do not accept this License.Therefore, by\n"
		"modifying or distributing the Program(or any work based on the\n"
		"Program), you indicate your acceptance of this License to do so, and\n"
		"all its terms and conditions for copying, distributing or modifying\n"
		"the Program or works based on it.\n\n"

		"6. Each time you redistribute the Program(or any work based on the\n"
		"Program), the recipient automatically receives a license from the\n"
		"original licensor to copy, distribute or modify the Program subject to\n"
		"these terms and conditions.You may not impose any further\n"
		"restrictions on the recipients' exercise of the rights granted herein.\n"
		"You are not responsible for enforcing compliance by third parties to\n"
		"this License.\n\n"

		"7. If, as a consequence of a court judgment or allegation of patent\n"
		"infringement or for any other reason(not limited to patent issues),\n"
		"conditions are imposed on you(whether by court order, agreement or\n"
		"otherwise) that contradict the conditions of this License, they do not\n"
		"excuse you from the conditions of this License.If you cannot\n"
		"distribute so as to satisfy simultaneously your obligations under this\n"
		"License and any other pertinent obligations, then as a consequence you\n"
		"may not distribute the Program at all.For example, if a patent\n"
		"license would not permit royalty-free redistribution of the Program by\n"
		"all those who receive copies directly or indirectly through you, then\n"
		"the only way you could satisfy both it and this License would be to\n"
		"refrain entirely from distribution of the Program.\n\n"

		"If any portion of this section is held invalid or unenforceable under\n"
		"any particular circumstance, the balance of the section is intended to\n"
		"apply and the section as a whole is intended to apply in other\n"
		"circumstances.\n\n"

		"It is not the purpose of this section to induce you to infringe any\n"
		"patents or other property right claims or to contest validity of any\n"
		"such claims; this section has the sole purpose of protecting the\n"
		"integrity of the free software distribution system, which is\n"
		"implemented by public license practices.Many people have made\n"
		"generous contributions to the wide range of software distributed\n"
		"through that system in reliance on consistent application of that\n"
		"system; it is up to the author / donor to decide if he or she is willing\n"
		"to distribute software through any other system and a licensee cannot\n"
		"impose that choice.\n\n"

		"This section is intended to make thoroughly clear what is believed to\n"
		"be a consequence of the rest of this License.\n\n"

		"8. If the distribution and/or use of the Program is restricted in\n"
		"certain countries either by patents or by copyrighted interfaces, the\n"
		"original copyright holder who places the Program under this License\n"
		"may add an explicit geographical distribution limitation excluding\n"
		"those countries, so that distribution is permitted only in or among\n"
		"countries not thus excluded.In such case, this License incorporates\n"
		"the limitation as if written in the body of this License.\n\n"

		"9. The Free Software Foundation may publish revised and/or new versions\n"
		"of the General Public License from time to time.Such new versions will\n"
		"be similar in spirit to the present version, but may differ in detail to\n"
		"address new problems or concerns.\n\n"

		"Each version is given a distinguishing version number.If the Program\n"
		"specifies a version number of this License which applies to it and any\n"
		"""later version"", you have the option of following the terms and conditions\n"
		"either of that version or of any later version published by the Free\n"
		"Software Foundation.If the Program does not specify a version number of\n"
		"this License, you may choose any version ever published by the Free Software\n"
		"Foundation.\n\n"

		"10. If you wish to incorporate parts of the Program into other free\n"
		"programs whose distribution conditions are different, write to the author\n"
		"to ask for permission.For software which is copyrighted by the Free\n"
		"Software Foundation, write to the Free Software Foundation; we sometimes\n"
		"make exceptions for this.Our decision will be guided by the two goals\n"
		"of preserving the free status of all derivatives of our free software and\n"
		"of promoting the sharing and reuse of software generally.\n\n"

		"NO WARRANTY\n\n"

		"11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n"
		"FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.EXCEPT WHEN\n"
		"OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND / OR OTHER PARTIES\n"
		"PROVIDE THE PROGRAM ""AS IS"" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n"
		"OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.THE ENTIRE RISK AS\n"
		"TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.SHOULD THE\n"
		"PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n"
		"REPAIR OR CORRECTION.\n\n"

		"12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n"
		"WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND / OR\n"
		"REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n"
		"INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n"
		"OUT OF THE USE OR INABILITY TO USE THE PROGRAM(INCLUDING BUT NOT LIMITED\n"
		"TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n"
		"YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n"
		"PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n"
		"POSSIBILITY OF SUCH DAMAGES.\n\n"

		"END OF TERMS AND CONDITIONS\n\n"

		"How to Apply These Terms to Your New Programs\n\n"

		"If you develop a new program, and you want it to be of the greatest\n"
		"possible use to the public, the best way to achieve this is to make it\n"
		"free software which everyone can redistribute and change under these terms.\n"

		"To do so, attach the following notices to the program.It is safest\n"
		"to attach them to the start of each source file to most effectively\n"
		"convey the exclusion of warranty; and each file should have at least\n"
		"the ""copyright"" line and a pointer to where the full notice is found.\n\n"

		"Sepand X-Ray Inspector (SXI1) Software 1.0.0 \n\n"
		"Copyright (C) 2014-2016  Alireza Alipour \n\n"

		"This program is free software; you can redistribute it and / or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation; either version 2 of the License, or\n"
		"(at your option) any later version.\n\n"

		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the\n"
		"GNU General Public License for more details.\n\n"

		"You should have received a copy of the GNU General Public License along\n"
		"with this program; if not, write to the Free Software Foundation, Inc.,\n"
		"51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.\n\n"

		"Also add information on how to contact you by electronic and paper mail.\n\n"

		"If the program is interactive, make it output a short notice like this\n"
		"when it starts in an interactive mode:\n\n"

		"Gnomovision version 69, Copyright(C) year name of author\n"
		"Gnomovision comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n"
		"This is free software, and you are welcome to redistribute it\n"
		"under certain conditions; type `show c' for details.\n\n"

		"The hypothetical commands `show w' and `show c' should show the appropriate\n"
		"parts of the General Public License.Of course, the commands you use may\n"
		"be called something other than `show w' and `show c'; they could even be\n"
		"mouse - clicks or menu items--whatever suits your program.\n\n"

		"You should also get your employer(if you work as a programmer) or your\n"
		"school, if any, to sign a ""copyright disclaimer"" for the program, if\n"
		"necessary.Here is a sample; alter the names:\n\n"

		"Yoyodyne, Inc., hereby disclaims all copyright interest in the program\n"
		"`Gnomovision' (which makes passes at compilers) written by James Hacker.\n\n"

		"&lt; signature of Ty Coon&gt; , 1 April 1989\n"
		"Ty Coon, President of Vice\n\n"

		"This General Public License does not permit incorporating your program into\n"
		"proprietary programs.If your program is a subroutine library, you may\n"
		"consider it more useful to permit linking proprietary applications with the\n"
		"library.If this is what you want to do, use the GNU Lesser General\n"
		"Public License instead of this License.";

	static GdkPixbuf* logo;
	gchar* name = "Sepand X-Ray Inspector (SXI1) Software";
	gchar* version = "1.0.0";
	//gchar* website = "http://www.sepand.com/";
	//gchar* website_label = "SXI1 (Sepand X-Ray Inspector)";

	logo = gdk_pixbuf_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/sepandbig.png", NULL);
	gtk_window_set_default_icon(logo);

	gtk_show_about_dialog(win,
		"authors", authors, "license", license, "license-type", GTK_LICENSE_CUSTOM,
		"comments", comments, "copyright", copyright,
		"logo", logo, "program-name",
		name, "version", version, NULL);
}

static void refopencv(GtkWindow *win)
{
	gchar* authors[] = { "Intel Corporation, Willow Garage," "Itseez <Itseez.com>", NULL };
	gchar* comments = { "OpenCV (Open Source Computer Vision) is a library of programming\n"
		"functions mainly aimed at real-time computer vision. The library is  \n"
		"cross-platform and free for use under the open-source BSD license. " };

	gchar* copyright = { "Copyright (c) 2016, Itseez." };

	const gchar *license = "By downloading, copying, installing or using the software you agree to this license.\n"
		"If you do not agree to this license, do not download, install,\n"
		"copy or use the software.\n\n"

		"License Agreement\n"
		"For Open Source Computer Vision Library\n"
		"(3 - clause BSD License)\n\n"

		"Copyright(C) 2000 - 2016, Intel Corporation, all rights reserved.\n"
		"Copyright(C) 2009 - 2011, Willow Garage Inc., all rights reserved.\n"
		"Copyright(C) 2009 - 2016, NVIDIA Corporation, all rights reserved.\n"
		"Copyright(C) 2010 - 2013, Advanced Micro Devices, Inc., all rights reserved.\n"
		"Copyright(C) 2015 - 2016, OpenCV Foundation, all rights reserved.\n"
		"Copyright(C) 2015 - 2016, Itseez Inc., all rights reserved.\n"
		"Third party copyrights are property of their respective owners.\n\n"

		"Redistribution and use in source and binary forms, with or without modification,\n"
		"are permitted provided that the following conditions are met:\n\n"

		"*Redistributions of source code must retain the above copyright notice,\n"
		"this list of conditions and the following disclaimer.\n\n"

		"* Redistributions in binary form must reproduce the above copyright notice,\n"
		"this list of conditions and the following disclaimer in the documentation\n"
		"and / or other materials provided with the distribution.\n\n"

		"* Neither the names of the copyright holders nor the names of the contributors\n"
		"may be used to endorse or promote products derived from this software\n"
		"without specific prior written permission.\n\n"

		"This software is provided by the copyright holders and contributors ""as is"" and\n"
		"any express or implied warranties, including, but not limited to, the implied\n"
		"warranties of merchantability and fitness for a particular purpose are disclaimed.\n"
		"In no event shall copyright holders or contributors be liable for any direct,\n"
		"indirect, incidental, special, exemplary, or consequential damages\n"
		"(including, but not limited to, procurement of substitute goods or services;\n"
		"loss of use, data, or profits; or business interruption) however caused\n"
		"and on any theory of liability, whether in contract, strict liability,\n"
		"or tort(including negligence or otherwise) arising in any way out of\n"
		"the use of this software, even if advised of the possibility of such damage.";

	static GdkPixbuf* logo;
	gchar* name = "Open Source Computer Vision (OpenCV)";
	gchar* version = "3.1.0";
	gchar* website = "http://www.opencv.org/";
	gchar* website_label = "OpenCV (Open Source Computer Vision)";

	logo = gdk_pixbuf_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/opencvbig.png", NULL);
	gtk_window_set_default_icon(logo);

	gtk_show_about_dialog(win,
		"authors", authors, "license", license, "license-type", GTK_LICENSE_CUSTOM,
		"comments", comments, "copyright", copyright,
		"logo", logo, "program-name",
		name, "version", version, "website", website,
		"website-label", website_label, NULL);
}

static void refgtk(GtkWindow *win)
{
	gchar* authors[] = {
		"Current core maintainers of GTK + are:\n"
		"--------------------------------------------------\n"
		"Name 	                                Affiliation\n"
		"--------------------------------------------------\n"
		"Matthias Clasen 	 	        Red Hat\n"
		"Behdad Esfahbod 	                Google\n"
		"Benjamin Otte 		        Red Hat\n"
		"Federico Mena Quintero 	Novell\n"
		"Alexander Larsson 	                Red Hat\n"
		"Tristan Van Berkom 	        Codethink\n"
		"Carlos Garnacho 	                Red Hat\n"
		"Kristian Rietveld 	                Lanedo GmbH\n"
		"Michael Natterer 	                Lanedo GmbH\n"
		"Ryan Lortie 	                        Canonical\n"
		"Emmanuele Bassi 	                Endless Mobile\n"
		, NULL };
 
	gchar* comments = { "GTK+ , or the GIMP Toolkit, is a multi-platform toolkit for creating graphical\n"
		"user interfaces.Offering a complete set of widgets, GTK + is suitable for     \n"
		"projects ranging from small one-off tools to complete application suites.     " };

	gchar* copyright = { "Copyright (c) 2007-2016 The GTK+ Team | Valid XHTML and CSS" };

	const gchar *license = "The MIT License (MIT)\n\n"

		"Copyright(c) 2013 - 2015, The Gtk - rs Project Developers.\n\n"

		"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
		"of this software and associated documentation files(the ""Software""), to deal\n"
		"in the Software without restriction, including without limitation the rights\n"
		"to use, copy, modify, merge, publish, distribute, sublicense, and / or sell\n"
		"copies of the Software, and to permit persons to whom the Software is\n"
		"furnished to do so, subject to the following conditions :\n\n"

		"The above copyright notice and this permission notice shall be included in all\n"
		"copies or substantial portions of the Software.\n\n"

		"THE SOFTWARE IS PROVIDED ""AS IS"", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
		"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE\n"
		"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
		"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
		"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
		"SOFTWARE.";

	static GdkPixbuf* logo;
	gchar* name = "GIMP Toolkit (GTK+)";
	gchar* version = "3.20.6";
	gchar* website = "http://www.gtk.org/";
	gchar* website_label = "The GTK+ Project";

	logo = gdk_pixbuf_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/gtkbig.png", NULL);
	gtk_window_set_default_icon(logo);

	gtk_show_about_dialog(win,
		"authors", authors, "license", license, "license-type", GTK_LICENSE_CUSTOM,
		"comments", comments, "copyright", copyright,
		"logo", logo, "program-name",
		name, "version", version, "website", website,
		"website-label", website_label, NULL);
}
//=================================================================================================================
//clear imgs
//=================================================================================================================
void clean(GtkWidget *widget, gpointer user_data)
{
	gtk_image_clear(GTK_IMAGE(img1));
	gtk_image_clear(GTK_IMAGE(img2));
	gtk_image_clear(GTK_IMAGE(img3));
	gtk_image_clear(GTK_IMAGE(img4));
	img1buffer = NULL;
}
//=================================================================================================================
// exit window
//=================================================================================================================
void exitwin(GtkWidget *widget, gpointer win)
{
	gtk_widget_destroy(GTK_WIDGET(win));
}
//==================================================================================================================
//new image 
//==================================================================================================================
void newimage(GtkWidget *widget, gpointer data)
{
	gtk_image_clear(GTK_IMAGE(img1));
	gtk_image_clear(GTK_IMAGE(img2));
	gtk_image_clear(GTK_IMAGE(img3));
	gtk_image_clear(GTK_IMAGE(img4));
	img1buffer = NULL;

	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Open your raw X-ray image from a file",
		GTK_WINDOW(win),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}

	gtk_widget_destroy(dialog);

	if (filename != NULL)
	{
		img1buffer = gdk_pixbuf_new_from_file(filename, NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(img1), img1buffer);
		width = gdk_pixbuf_get_width(img1buffer);
		height = gdk_pixbuf_get_height(img1buffer);
	}
}
//=======================================================================================================
//resize 
//=======================================================================================================
void resize(GtkWidget *widget, gpointer user_data, GtkWidget *button, GtkWindow *win)
{
	static GdkPixbuf* logo;
	logo = gdk_pixbuf_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/resize.png", NULL);
	gtk_window_set_default_icon(logo);

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Resize",
		win, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, NULL);

	///////////Width////
	GtkWidget *dialog_content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	const char *message = "<b>Width</b>";
	GtkWidget *label1 = gtk_label_new(message);
	gtk_misc_set_alignment(GTK_MISC(label1), 0, 1);
	gtk_label_set_markup(GTK_LABEL(label1), message);
	gtk_box_pack_start(GTK_BOX(dialog_content_area), label1, TRUE, TRUE, 5);

	width_combo = gtk_combo_box_text_new_with_entry();
	for (gint i = 1; i <= 7; i++)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(width_combo), g_strdup_printf("%d", i * 100));
	    gtk_combo_box_set_active(GTK_COMBO_BOX(width_combo), 0);
	    gtk_box_pack_start(GTK_BOX(dialog_content_area), width_combo, TRUE, TRUE, 5);

	//////////Height//////////
	const char *message2 = "<b>Height</b>";
	GtkWidget *label2 = gtk_label_new(message2);
	gtk_misc_set_alignment(GTK_MISC(label2), 0, 1);
	gtk_label_set_markup(GTK_LABEL(label2), message2);
	gtk_box_pack_start(GTK_BOX(dialog_content_area), label2, TRUE, TRUE, 5);

	height_combo = gtk_combo_box_text_new_with_entry();
	for (gint i = 1; i <= 3; i++)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(height_combo), g_strdup_printf("%d", i * 100));
	    gtk_combo_box_set_active(GTK_COMBO_BOX(height_combo), 0);
	    gtk_box_pack_start(GTK_BOX(dialog_content_area), height_combo, TRUE, TRUE, 5);

	///////////////
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY);
	gtk_widget_show_all(dialog);


	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_APPLY) {

		if (img1buffer == NULL)
			return;
		width = atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(width_combo)));
		height = atoi(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(height_combo)));
		set_img_zoom();
	}

	gtk_widget_destroy(dialog);

}
//=======================================================================================================
//Undo-Redo operation
//=======================================================================================================
void undoredo(GtkWidget *widget, gpointer data) 
{
		img1buffer = NULL;
		img1buffer = gdk_pixbuf_new_from_file(filename, NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(img1), img1buffer);
		width = gdk_pixbuf_get_width(img1buffer);
		height = gdk_pixbuf_get_height(img1buffer);
}
//=======================================================================================================
//Print operation
//=======================================================================================================
void draw_page(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer selected_image)
{
	cairo_t *cr;
	gdouble max_width, max_height;
	gdouble coeff;
	GdkPixbuf *img = nullptr, *logo;


	cr = gtk_print_context_get_cairo_context(context);
	max_width = gtk_print_context_get_width(context);
	max_height = gtk_print_context_get_height(context);


	if (height > width)
		coeff = (max_height - 600) / height;
	else
		coeff = max_width / width;

	switch (GPOINTER_TO_SIZE(selected_image))
	{
	case 0:
		img = gdk_pixbuf_scale_simple(img1buffer, width*coeff, height*coeff, GDK_INTERP_NEAREST);
		break;
	case 1:
		img = gdk_pixbuf_scale_simple(gtk_image_get_pixbuf(GTK_IMAGE(img2)), width*coeff, height*coeff, GDK_INTERP_NEAREST);
		break;
	case 2:
		img = gdk_pixbuf_scale_simple(gtk_image_get_pixbuf(GTK_IMAGE(img3)), width*coeff, height*coeff, GDK_INTERP_NEAREST);
		break;
	case 3:
		img = gdk_pixbuf_scale_simple(gtk_image_get_pixbuf(GTK_IMAGE(img4)), width*coeff, height*coeff, GDK_INTERP_NEAREST);
		break;

	default:
		break;
	}


	logo = gdk_pixbuf_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/sepandbig.png", NULL);
	logo = gdk_pixbuf_scale_simple(logo, 500, 800, GDK_INTERP_NEAREST);

	gdk_cairo_set_source_pixbuf(cr, img, 0, 900);
	cairo_paint(cr);

	gdk_cairo_set_source_pixbuf(cr, logo, 0, max_height - 500);
	cairo_paint(cr);

	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_select_font_face(cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 100);
	cairo_move_to(cr, 600, max_height - 200);
	cairo_show_text(cr, "Copyright (c) 2016, Sepand X-ray Inspector (SXI1). All rights reserved.");

	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_select_font_face(cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 100);
	cairo_move_to(cr, 0, max_height - 600);
	cairo_show_text(cr, "*-----------------------------------------------------------------------------------------------------------------------------------------------*");
}

void print(GtkWidget *widget, gpointer win)
{
	GtkWidget *print_dialog = (GtkWidget *)gtk_builder_get_object(printui, "print_dialog");
	gtk_combo_box_set_active((GtkComboBox *)gtk_builder_get_object(printui, "print_selected_image"), 0);
	gint result = gtk_dialog_run(GTK_DIALOG(print_dialog));
	gtk_widget_hide(print_dialog);
	gint selected_image = gtk_combo_box_get_active((GtkComboBox *)gtk_builder_get_object(printui, "print_selected_image"));

	if (selected_image == 0 && img1buffer == NULL)
		return;
	if (selected_image == 1 && gtk_image_get_pixbuf(GTK_IMAGE(img2)) == NULL)
		return;
	if (selected_image == 2 && gtk_image_get_pixbuf(GTK_IMAGE(img3)) == NULL)
		return;
	if (selected_image == 3 && gtk_image_get_pixbuf(GTK_IMAGE(img4)) == NULL)
		return;


	if (result == GTK_RESPONSE_OK)
	{
		GtkPrintOperation *print;
		GtkPrintOperationResult res1;
		GtkPrintSettings *settings = NULL;

		print = gtk_print_operation_new();
		if (settings != NULL)
			gtk_print_operation_set_print_settings(print, settings);
		gtk_print_operation_set_n_pages(print, 1);
		gtk_print_operation_set_unit(print, GTK_UNIT_PIXEL);

		g_signal_connect(print, "draw_page", G_CALLBACK(draw_page), GSIZE_TO_POINTER(selected_image));

		res1 = gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW(win), NULL);
		if (res1 == GTK_PRINT_OPERATION_RESULT_APPLY)
		{
			if (settings != NULL)
				g_object_unref(settings);
			settings = (GtkPrintSettings *)g_object_ref(gtk_print_operation_get_print_settings(print));
		}
		g_object_unref(print);
	}
}
//=======================================================================================================
//sxigraph open when user and pass is incorrect
//=======================================================================================================
double sxigraph(GtkWidget *widget, int argc, char *argv[])
{

	//Username and Password to validate credentials
	const gchar *USERNAME = "Sepand";
	const gchar *PASSWORD = "@#Sepand3412!*@";

	if (!g_strcmp0(USERNAME, gtk_entry_get_text(GTK_ENTRY(u_name))) && !g_strcmp0(PASSWORD, gtk_entry_get_text(GTK_ENTRY(pass))))
	{

	gtk_init(&argc, &argv);
	GError *uierror = NULL;
	printui = gtk_builder_new();
	guint err = gtk_builder_add_from_file(printui, "C:/Program Files (x86)/Sepand64bit/logo/printui.glade", &uierror);
	if (!err) {
		g_print("%s", uierror->message);
		return 1;
	}
	gtk_builder_connect_signals(printui, NULL);

	/******************WINDOW***********************************/
	gtk_widget_destroy(window);
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_icon_from_file(GTK_WINDOW(win), "C:/Program Files (x86)/Sepand64bit/logo/sepand.png", NULL);
	gtk_window_set_title(GTK_WINDOW(win), "Sepand X-Ray Inspector (SXI1)");
	gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(win), 0);
	gtk_window_maximize(GTK_WINDOW(win));
	gdk_color_parse("#FFFFFF", &color);
	gtk_widget_modify_bg(GTK_WIDGET(win), GTK_STATE_NORMAL, &color);

	/******************VBOX***********************************/
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 0);

	/**********TABLE****************************************************/
	GtkWidget *table = gtk_table_new(18, 18, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 2);
	gtk_container_add(GTK_CONTAINER(win), table);

	gtk_table_attach(GTK_TABLE(table), vbox, 0, 18, 0, 4, GTK_FILL, GTK_FILL, 0, 0);

	/**********MENUBAR*************************************************/
	GtkWidget *menubar = gtk_menu_bar_new();
	gtk_container_set_border_width(GTK_CONTAINER(menubar), 0);

	/*****TOOLBAR***********************************************************/
	GtkWidget *toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_container_set_border_width(GTK_CONTAINER(toolbar), 0);
	gtk_table_attach(GTK_TABLE(vbox), toolbar, 0, 18, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	////File////////
	GtkWidget *fileMenu = gtk_menu_new();
	fileMi = gtk_menu_item_new_with_mnemonic("_File");
	gdk_color_parse("#80FF80", &color);
	gtk_widget_modify_bg(fileMi, GTK_STATE_NORMAL, &color);

	//HOT KEY
	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(win), accel_group);
	//NEW
	GtkWidget *newMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, accel_group);
	gtk_widget_add_accelerator(newMi, "activate", accel_group, GDK_KEY_N, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	GtkToolItem *newTb = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), newTb, -1);
	gtk_tool_item_set_tooltip_text(newTb, "New");

	//OPEN
	GtkWidget *openMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, accel_group);
	gtk_widget_add_accelerator(openMi, "activate", accel_group, GDK_KEY_O, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	GtkToolItem *openTb = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), openTb, -1);
	gtk_tool_item_set_tooltip_text(openTb, "Open");

	//SAVEAS
	GtkWidget *saveMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE_AS, accel_group);
	gtk_widget_add_accelerator(saveMi, "activate", accel_group, GDK_KEY_S, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	GtkToolItem *saveTb = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE_AS);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), saveTb, -1);

	gtk_tool_item_set_tooltip_text(saveTb, "Save");
	//PRINT
	GtkWidget *printMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_PRINT, accel_group);
	gtk_widget_add_accelerator(printMi, "activate", accel_group, GDK_KEY_P, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	GtkWidget *print_dialog = (GtkWidget *)gtk_builder_get_object(printui, "print_dialog");

	GtkToolItem *printTb = gtk_tool_button_new_from_stock(GTK_STOCK_PRINT);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), printTb, -1);
	gtk_tool_item_set_tooltip_text(printTb, "Print");

	////EXIT
	GtkWidget *separ = gtk_separator_menu_item_new();

	GtkWidget *quitMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);
	gtk_widget_add_accelerator(quitMi, "activate", accel_group, GDK_KEY_Q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);


	GtkToolItem *sepa = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sepa, -1);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), newMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), printMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), separ);
	gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);

	////Edit image////////
	GtkWidget *editMenu = gtk_menu_new();
	GtkWidget *editMi = gtk_menu_item_new_with_mnemonic("_Edit");
	gdk_color_parse("#f58388", &color);
	gtk_widget_modify_bg(editMi, GTK_STATE_NORMAL, &color);

	//Resize;
	GtkWidget *imageResize = gtk_image_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/resize.png");
	GtkWidget *resizeMi = gtk_image_menu_item_new_from_stock("_Resize", accel_group);
	gtk_widget_add_accelerator(resizeMi, "activate", accel_group, GDK_KEY_R, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(resizeMi), GTK_WIDGET(imageResize));

	//Cut;
	GtkWidget *cropMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_CUT, accel_group);
	gtk_widget_add_accelerator(cropMi, "activate", accel_group, GDK_KEY_X, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	//Rotate;

	GtkWidget *imageRotate = gtk_image_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/rotate.png");
	GtkWidget *rotateMi = gtk_image_menu_item_new_from_stock("_Rotate", accel_group);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(rotateMi), GTK_WIDGET(imageRotate));

	rotateMi1 = gtk_menu_item_new_with_label("+90 ");
	rotateMi2 = gtk_menu_item_new_with_label("-90");
	rotateMi3 = gtk_menu_item_new_with_label("+180");
	rotateMi4 = gtk_menu_item_new_with_label("-180");

	imprMenu = gtk_menu_new();

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(editMi), editMenu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(rotateMi), imprMenu);

	gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), resizeMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), cropMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), rotateMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(imprMenu), rotateMi1);
	gtk_menu_shell_append(GTK_MENU_SHELL(imprMenu), rotateMi2);
	gtk_menu_shell_append(GTK_MENU_SHELL(imprMenu), rotateMi3);
	gtk_menu_shell_append(GTK_MENU_SHELL(imprMenu), rotateMi4);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), editMi);


	//undo-undo;
	GtkWidget *separr = gtk_separator_menu_item_new(); 

	GtkWidget *undoMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_UNDO, accel_group);
	gtk_widget_add_accelerator(undoMi, "activate", accel_group, GDK_KEY_Z, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	GtkToolItem *undo = gtk_tool_button_new_from_stock(GTK_STOCK_UNDO);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), undo, -1);
	gtk_tool_item_set_tooltip_text(undo, "Undo");


	GtkWidget *redoMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_REDO, accel_group);
	gtk_widget_add_accelerator(redoMi, "activate", accel_group, GDK_KEY_Y, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	GtkToolItem *redo = gtk_tool_button_new_from_stock(GTK_STOCK_REDO);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), redo, -1);
	gtk_tool_item_set_tooltip_text(redo, "Redo");

	GtkToolItem *seppp = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), seppp, -1);


	gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), separr);
	gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), redoMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), undoMi);


	////Tools////////
	GtkWidget *BuildMenu = gtk_menu_new();
	GtkWidget *BuildMi = gtk_menu_item_new_with_mnemonic("_Tools");
	gdk_color_parse("#00a3e8", &color);
	gtk_widget_modify_bg(BuildMi, GTK_STATE_NORMAL, &color);

	//Run;
	GtkWidget *runMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, accel_group);
	gtk_widget_add_accelerator(runMi, "activate", accel_group, GDK_KEY_U, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	GtkToolItem *runTb = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), runTb, -1);
	gtk_tool_item_set_tooltip_text(runTb, "Compile");
	//CLEAR;
	GtkWidget *clearMi = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, accel_group);
	gtk_widget_add_accelerator(clearMi, "activate", accel_group, GDK_KEY_D, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	GtkToolItem *clearTb = gtk_tool_button_new_from_stock(GTK_STOCK_CLEAR);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), clearTb, -1);
	gtk_tool_item_set_tooltip_text(clearTb, "Clear");

	GtkToolItem *sepa2 = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sepa2, -1);

	GtkToolItem *quitTb = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), quitTb, -1);
	gtk_tool_item_set_tooltip_text(quitTb, "Quit");

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(BuildMi), BuildMenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(BuildMenu), runMi);

	gtk_menu_shell_append(GTK_MENU_SHELL(BuildMenu), clearMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), BuildMi);

	/////Help///////
	GtkWidget *helpMenu = gtk_menu_new();
	GtkWidget *helpMi = gtk_menu_item_new_with_mnemonic("_Help");
	gdk_color_parse("#fef200", &color);
	gtk_widget_modify_bg(helpMi, GTK_STATE_NORMAL, &color);

	//SXI1
	GtkWidget *imageSXI, *sxiMi;
	imageSXI = gtk_image_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/sepandsmall.png");
	sxiMi = gtk_image_menu_item_new_with_mnemonic("About SXI1...");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(sxiMi), GTK_WIDGET(imageSXI));

	//OPENCV
	GtkWidget *imageOpenCV, *opencvMi;
	imageOpenCV = gtk_image_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/opencv.png");
	opencvMi = gtk_image_menu_item_new_with_mnemonic("About OpenCV...");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(opencvMi), GTK_WIDGET(imageOpenCV));

	//GTK+
	GtkWidget *imageGTK, *gtkMi;
	imageGTK = gtk_image_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/gtk.png");
	gtkMi = gtk_image_menu_item_new_with_mnemonic("About GTK+...");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(gtkMi), GTK_WIDGET(imageGTK));

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpMi), helpMenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), sxiMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), opencvMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), gtkMi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), helpMi);


	g_signal_connect(G_OBJECT(openMi), "activate", G_CALLBACK(browse), NULL);
	g_signal_connect(G_OBJECT(openTb), "clicked", G_CALLBACK(browse), NULL);
	
	g_signal_connect(G_OBJECT(saveMi), "activate", G_CALLBACK(save), NULL);
	g_signal_connect(G_OBJECT(saveTb), "clicked", G_CALLBACK(save), (gpointer)win);

	g_signal_connect(G_OBJECT(printMi), "activate", G_CALLBACK(print), (gpointer)win);
	g_signal_connect(G_OBJECT(printTb), "clicked", G_CALLBACK(print), NULL);

	g_signal_connect(G_OBJECT(newMi), "activate", G_CALLBACK(newimage), NULL);
	g_signal_connect(G_OBJECT(newTb), "clicked", G_CALLBACK(newimage), NULL);

	g_signal_connect(G_OBJECT(quitMi), "activate", G_CALLBACK(exitwin), (gpointer)win);
	g_signal_connect(G_OBJECT(quitTb), "clicked", G_CALLBACK(exitwin), (gpointer)win);

	g_signal_connect(G_OBJECT(resizeMi), "activate", G_CALLBACK(resize), NULL);
	g_signal_connect(G_OBJECT(rotateMi1), "activate", G_CALLBACK(rotation), 0);
	g_signal_connect(G_OBJECT(rotateMi2), "activate", G_CALLBACK(rotation), gpointer(1));
	g_signal_connect(G_OBJECT(rotateMi3), "activate", G_CALLBACK(rotation), gpointer(2));
	g_signal_connect(G_OBJECT(rotateMi4), "activate", G_CALLBACK(rotation), gpointer(3));

	g_signal_connect(G_OBJECT(cropMi), "activate", G_CALLBACK(crop), (gpointer)win);

	g_signal_connect(G_OBJECT(undoMi), "activate", G_CALLBACK(undoredo), NULL);
	g_signal_connect(G_OBJECT(redoMi), "activate", G_CALLBACK(undoredo), NULL);
	g_signal_connect(G_OBJECT(undo), "clicked", G_CALLBACK(undoredo), NULL);
	g_signal_connect(G_OBJECT(redo), "clicked", G_CALLBACK(undoredo), NULL);

	g_signal_connect(G_OBJECT(clearMi), "activate", G_CALLBACK(clean), NULL);
	g_signal_connect(G_OBJECT(clearTb), "clicked", G_CALLBACK(clean), NULL);

	g_signal_connect(G_OBJECT(runMi), "activate", G_CALLBACK(SXI1), NULL);
	g_signal_connect(G_OBJECT(runTb), "clicked", G_CALLBACK(SXI1), NULL);

	g_signal_connect(G_OBJECT(sxiMi), "activate", G_CALLBACK(refsxi), NULL);
	g_signal_connect(G_OBJECT(opencvMi), "activate", G_CALLBACK(refopencv), NULL);
	g_signal_connect(G_OBJECT(gtkMi), "activate", G_CALLBACK(refgtk), NULL);
	/*******************************************************************/

	/* create a new scrolled window. */
	GtkAdjustment*adj1 = (GtkAdjustment*)gtk_adjustment_new(0, 0, 50, 0, 0, 0);
	GtkAdjustment*adj2 = (GtkAdjustment*)gtk_adjustment_new(0, 0, 50, 0, 0, 0);

	GtkWidget *scrolled_window = gtk_scrolled_window_new(adj1, adj2);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	gtk_table_attach_defaults(GTK_TABLE(table), scrolled_window, 0, 9, 3, 10);

	frame1 = bold_frame("Raw X-Ray Image");
	gtk_frame_set_shadow_type(GTK_FRAME(frame1), GTK_SHADOW_IN);
	gtk_table_attach_defaults(GTK_TABLE(table), frame1, 0, 9, 2, 10);


	frame2 = bold_frame("Subtracted X-Ray Image");
	gtk_frame_set_shadow_type(GTK_FRAME(frame2), GTK_SHADOW_OUT);
	gtk_table_attach_defaults(GTK_TABLE(table), frame2, 9, 18, 2, 10);

	frame3 = bold_frame("Denoised X-Ray Image");
	gtk_frame_set_shadow_type(GTK_FRAME(frame3), GTK_SHADOW_ETCHED_IN);
	gtk_table_attach_defaults(GTK_TABLE(table), frame3, 9, 18, 10, 18);

	frame4 = bold_frame("Pseudo Colored X-Ray Image");
	gtk_frame_set_shadow_type(GTK_FRAME(frame4), GTK_SHADOW_ETCHED_IN);
	gtk_table_attach_defaults(GTK_TABLE(table), frame4, 0, 9, 10, 18);
	/*******************************************************************/

	colorize_combo = gtk_combo_box_text_new_with_entry();

	const char *texts[] = { "AUTUMN", "BONE", "JET", "WINTER",
		"RAINBOW", "OCEAN", "SUMMER", "SPRING",
		"COOL", "HSV", "PINK", "HOT", "PARULA" };

	for (int i = 0; i < G_N_ELEMENTS(texts); i++)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(colorize_combo), texts[i]);

	gtk_combo_box_set_active(GTK_COMBO_BOX(colorize_combo), 11);
	gtk_table_attach_defaults(GTK_TABLE(table), colorize_combo, 0, 1, 17, 18);

	but13 = bold_img_button("Color", "C:/Program Files (x86)/Sepand64bit/logo/colorize.png");
	gdk_color_parse("#50a0ff", &color);
	gtk_widget_modify_bg(but13, GTK_STATE_NORMAL, &color);
	gtk_table_attach_defaults(GTK_TABLE(table), but13, 1, 2, 17, 18);
	g_signal_connect(G_OBJECT(but13), "clicked", G_CALLBACK(SXI3), "colorized");
	//////
	threshold_combo = gtk_combo_box_text_new_with_entry();
	for (int i = 50; i <= 255; i += 10)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(threshold_combo), g_strdup_printf("%d", i));
	gtk_combo_box_set_active(GTK_COMBO_BOX(threshold_combo), 15);
	gtk_table_attach_defaults(GTK_TABLE(table), threshold_combo, 16, 17, 9, 10);

	but3 = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gdk_color_parse("#80FF80", &color);
	gtk_widget_modify_bg(but3, GTK_STATE_NORMAL, &color);
	gtk_table_attach_defaults(GTK_TABLE(table), but3, 17, 18, 9, 10);
	g_signal_connect(G_OBJECT(but3), "clicked", G_CALLBACK(SXI2), "threshold");
	/////
	fast_combo = gtk_combo_box_text_new_with_entry();
	for (int i = 1; i <= 21; i += 2)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fast_combo), g_strdup_printf("%d", i));
	gtk_combo_box_set_active(GTK_COMBO_BOX(fast_combo), 3);
	gtk_table_attach_defaults(GTK_TABLE(table), fast_combo, 16, 17, 17, 18);

	but3 = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gdk_color_parse("#80FF80", &color);
	gtk_widget_modify_bg(but3, GTK_STATE_NORMAL, &color);
	gtk_table_attach_defaults(GTK_TABLE(table), but3, 17, 18, 17, 18);
	g_signal_connect(G_OBJECT(but3), "clicked", G_CALLBACK(SXI2), "fast");

	/*************************************************************************************************/
	rotate_combo = gtk_combo_box_text_new_with_entry();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotate_combo), "+90");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotate_combo), "-90");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotate_combo), "+180");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(rotate_combo), "-180");

	/*************************************************************************************************/
	img1 = gtk_image_new();
	event_box = gtk_event_box_new();
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), event_box);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
	gtk_container_add(GTK_CONTAINER(event_box), img1);
	gtk_container_add(GTK_CONTAINER(frame1), event_box);
	

	g_signal_connect(G_OBJECT(event_box), "button_press_event", G_CALLBACK(mouse_press_callback), NULL);
	g_signal_connect(G_OBJECT(event_box), "button-release-event", G_CALLBACK(mouse_press_callback), NULL);


	img2 = gtk_image_new();
	gtk_table_attach_defaults(GTK_TABLE(table), img2, 9, 18, 2, 10);

	img3 = gtk_image_new();
	gtk_table_attach_defaults(GTK_TABLE(table), img3, 9, 18, 10, 18);

	img4 = gtk_image_new();
	gtk_table_attach_defaults(GTK_TABLE(table), img4, 0, 9, 10, 18);
	/******************************************************************/
	gtk_widget_show_all(win);

	gtk_main();

	}
	else
	{
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			"The username or password is incorrect.\n"
			"Please insert correct them.");
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

	}

}


int main(int argc, char *argv[]){

	GtkWidget *Login_button, *Quit_button;
	GtkWidget *label_user, *label_pass;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_icon_from_file(GTK_WINDOW(window), "C:/Program Files (x86)/Sepand64bit/logo/sepand.png", NULL);
	gtk_window_set_title(GTK_WINDOW(window), "User Login page");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gdk_color_parse("#a6ddf3", &color);
	gtk_widget_modify_bg(GTK_WIDGET(window), GTK_STATE_NORMAL, &color);

	GtkWidget *table = gtk_table_new(4, 4, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 7);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_container_add(GTK_CONTAINER(window), table);


	GtkWidget *frame = bold_frame("Log in to Sepand Software");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 6, 0, 9);


	GtkWidget *myimage2 = gtk_image_new_from_file("C:/Program Files (x86)/Sepand64bit/logo/key.png");
	gtk_table_attach_defaults(GTK_TABLE(table), myimage2, 0, 1, 0, 8);


	label_user = gtk_label_new("                                        Username*:");
	label_pass = gtk_label_new("                                        Password*:");

	u_name = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(u_name), "Username");
	gtk_table_attach_defaults(GTK_TABLE(table), u_name, 1, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(table), label_user, 0, 1, 3, 4);


	pass = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(pass), "Password");
	gtk_entry_set_visibility(GTK_ENTRY(pass), 0);
	gtk_table_attach_defaults(GTK_TABLE(table), pass, 1, 2, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(table), label_pass, 0, 1, 4, 5);


	Login_button = gtk_button_new_with_label("Log in");
	gdk_color_parse("green", &color);
	gtk_widget_modify_bg(GTK_WIDGET(Login_button), GTK_STATE_NORMAL, &color);
	g_signal_connect(Login_button, "clicked", G_CALLBACK(sxigraph), NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), Login_button, 1, 2, 5, 6);


	Quit_button = gtk_button_new_with_label("Quit");
	gdk_color_parse("red", &color);
	gtk_widget_modify_bg(GTK_WIDGET(Quit_button), GTK_STATE_NORMAL, &color);
	g_signal_connect(Quit_button, "clicked", G_CALLBACK(gtk_main_quit), NULL);
	gtk_table_attach_defaults(GTK_TABLE(table), Quit_button, 1, 2, 6, 7);


	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}