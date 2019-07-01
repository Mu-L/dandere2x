//
// Created by https://github.com/CardinalPanda
//
//Licensed under the GNU General Public License Version 3 (GNU GPL v3),
//    available at: https://www.gnu.org/licenses/gpl-3.0.txt

#ifndef DANDERE2X_DRIVER_H
#define DANDERE2X_DRIVER_H

#include "Plugins/Correction/Correction.h"
#include "Plugins/PFrame/PFrame.h"
#include "Plugins/Fade/Fade.h"

#include "Dandere2xUtils/Dandere2xUtils.h"
#include "Image/DebugImage/DebugImage.h"



/**
 * Todo:
 * - Simplify this driver class
 * - Add individual testing wrapper for debugging
 */


/**
 * Description:
 *
 * This is like the control room for Dandere2x - if you wish to add more additions to Dandere2x on the c++ side,
 * this where it is going to do it here.  Block matching, quality control, and saving of vectors all happen here.
 *
 *
 * Overview:
 *
 * - First we check if this is a resume frame, if it is, manually save the files as empty
 *   to create a p_frame at the resume frames position
 *
 *  //5-11-19  - this part is being overhauled - comments are outdated
 *
 */
using namespace dandere2x;
using namespace std;
const int correction_block_size = 2;


void driver_difference(string workspace, int resume_count, int frame_count,
                       int block_size, int step_size, string extension_type)  {


    // Prefixes for all the plugins. Makes the code more readible and less error prone to
    // just refer to them here.
    string image_prefix = workspace + separator() + "inputs" + separator() + "frame";
    string p_data_prefix = workspace + separator() + "pframe_data" + separator() + "pframe_";
    string difference_prefix = workspace + separator() + "inversion_data" + separator() + "inversion_";
    string correction_prefix = workspace + separator() + "correction_data" + separator() + "correction_";
    string fade_prefix = workspace + separator() + "fade_data" + separator() + "fade_";
    string compressed_prefix = workspace + separator() + "compressed" + separator() + "compressed_";

    // this is where d2x_cpp starts

    wait_for_file(image_prefix + to_string(1) + extension_type); //ensure the first file exists before starting d2x_cpp
    shared_ptr<Image> im1 = make_shared<Image>(image_prefix + to_string(1) + extension_type);

    // If the driver call is a resume case, handle it here.
    // The current Dandere2x implementation for resumes is to treat the next predicted frame as a new 'i' frame.
    if (resume_count != 1) {

        shared_ptr<Image> im2 = make_shared<Image>(image_prefix + to_string(resume_count + 1) + extension_type);

        string p_data_file = p_data_prefix + to_string(resume_count) + ".txt";
        string difference_file = difference_prefix + to_string(resume_count) + ".txt";
        string correction_file = correction_prefix + to_string(resume_count) + ".txt";
        string fade_file = fade_prefix + to_string(resume_count) + ".txt";

        write_empty(p_data_file);
        write_empty(difference_file);
        write_empty(correction_file);
        write_empty(fade_file);

        im1 = im2;

        resume_count++;
    }


    //Run our plugins for every frame in the video, starting at resume_count.
    
    for (int x = resume_count; x < frame_count; x++) {
        cout << "\n\n Computing differences for frame" << x << endl;

        // Load files needed for this for loop iteration
        string im2_file = image_prefix + to_string(x + 1) + extension_type;
        string im2_file_compressed = compressed_prefix + to_string(x + 1) + extension_type;


        // load actual images themselves
        shared_ptr<Image> im2 = make_shared<Image>(im2_file);
        shared_ptr<Image> im2_copy = make_shared<Image>(im2_file); //for corrections
        shared_ptr<Image> im2_compressed = make_shared<Image>(im2_file_compressed);

        // put into string the locations of where files will be written
        string p_data_file = p_data_prefix + to_string(x) + ".txt";
        string difference_file = difference_prefix + to_string(x) + ".txt";
        string correction_file = correction_prefix + to_string(x) + ".txt";
        string fade_file = fade_prefix + to_string(x) + ".txt";

        /** Run dandere2xCpp Plugins (this is where all the computation of d2xcpp happens)
         *
         *  Note:
         *  - The order the plugins are called are very significant.
         * */

        Fade fade = Fade(im1, im2, im2_compressed, block_size, fade_file);
        fade.run();

        PFrame pframe = PFrame(im1, im2, im2_compressed, block_size, p_data_file, difference_file, step_size);
        pframe.run();

        Correction correction = Correction(im2, im2_copy, im2_compressed, correction_block_size, correction_file, 2);
        correction.run();

        //For Debugging. Create a folder called 'debug_frames' in workspace when testing this
//        DebugImage before = DebugImage::create_debug_from_image(*im2);
//        before.save(workspace + "debug_frames" + separator() + "before_" + to_string(x) + ".png");

        /** Save the files generated by our plugins here */

        pframe.save();
        fade.save();
        correction.save();

        im1 = im2;
    }
}

#endif //DANDERE2X_DRIVER_H
