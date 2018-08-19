#include "libmv_capi.h"
#include <opencv2/calib3d.hpp>
#include <opencv2/core/eigen.hpp>
#include <postproc/medianer.h>
#include <GL/freeglut.h>

#define DATASET_OBJ "1529165036844.obj"
#define DATASET_PATH "dataset"

oc::Medianer* medianer;
int pose = 0;

void reconTest()
{
    float cx, cy, fx, fy;
    oc::Dataset dataset(DATASET_PATH);
    dataset.GetCalibration(cx, cy, fx, fy);

    std::vector<cv::String> images_paths;
    for (int i = 0; i < medianer->GetPoseCount(); i++)
        images_paths.push_back(dataset.GetFileName(i, ".jpg"));

    // Set reconstruction options
    libmv_ReconstructionOptions reconstruction_options(1, 2, libmv::BUNDLE_FOCAL_LENGTH | libmv::BUNDLE_PRINCIPAL_POINT | libmv::BUNDLE_RADIAL_K1 | libmv::BUNDLE_RADIAL_K2, 1, -1);
    libmv_CameraIntrinsicsOptions camera_instrinsic_options = libmv_CameraIntrinsicsOptions(libmv::DISTORTION_MODEL_POLYNOMIAL, fx, fy, cx, cy, 0, 0, 0);

    // Perform reconstruction
    libmv_Reconstruction libmv_reconstruction_ = *libmv_solveReconstructionImpl(images_paths, &camera_instrinsic_options, &reconstruction_options);
    const size_t n_views = libmv_reconstruction_.reconstruction.AllCameras().size();

    Matx33d R;
    Vec3d t;
    cv::Mat cvToGl = cv::Mat::zeros(4, 4, CV_64F);
    cvToGl.at<double>(0, 0) = 1.0f;
    cvToGl.at<double>(1, 1) = -1.0f; // Invert the y axis
    cvToGl.at<double>(2, 2) = -1.0f; // invert the z axis
    cvToGl.at<double>(3, 3) = 1.0f;
    for(size_t i = 0; i < n_views; ++i)
    {
      cv::eigen2cv(libmv_reconstruction_.reconstruction.AllCameras()[i].R, R);
      cv::eigen2cv(libmv_reconstruction_.reconstruction.AllCameras()[i].t, t);

      //http://answers.opencv.org/question/23089/opencv-opengl-proper-camera-pose-using-solvepnp/
      cv::Mat rotation, viewMatrix(4, 4, CV_64F);
      cv::Rodrigues(R, rotation);
      for(unsigned int row=0; row<3; ++row)
      {
         for(unsigned int col=0; col<3; ++col)
            viewMatrix.at<double>(row, col) = rotation.at<double>(row, col);
         viewMatrix.at<double>(row, 3) = t[row];
      }
      viewMatrix.at<double>(3, 3) = 1.0f;
      viewMatrix = cvToGl * viewMatrix;
      cv::Mat glViewMatrix = cv::Mat::zeros(4, 4, CV_64F);
      cv::transpose(viewMatrix , glViewMatrix);
      double* matrix = &glViewMatrix.at<double>(0, 0);
      glm::mat4 pose = dataset.GetPose(i)[0];
      LOGI("Camera %d: %lf %lf %lf vs %f %f %f", i, matrix[12], matrix[13], matrix[14], pose[3][0], pose[3][1], pose[3][2]);
    }
    LOGI("Cameras found: %d", n_views);
    std::exit(0);
}

void display(void) {
    medianer->RenderTexture(pose++);
    medianer->RenderPose(5);

    if (pose > medianer->GetPoseCount())
        pose = 0;
    glutSwapBuffers();
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(960, 540);
    glutInitContextVersion(3,0);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("Test");
    glutDisplayFunc(display);

    medianer = new oc::Medianer(DATASET_PATH, DATASET_OBJ, true);
    reconTest();
    glutMainLoop();
    return 0;
}
