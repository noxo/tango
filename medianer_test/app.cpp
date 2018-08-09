#include "libmv_capi.h"
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

    /*Rs.create(n_views, 1, CV_64F);
    Ts.create(n_views, 1, CV_64F);

    Matx33d R;
    Vec3d t;
    for(size_t i = 0; i < n_views; ++i)
    {
      eigen2cv(libmv_reconstruction_.reconstruction.AllCameras()[i].R, R);
      eigen2cv(libmv_reconstruction_.reconstruction.AllCameras()[i].t, t);
      Mat(R).copyTo(Rs.getMatRef(i));
      Mat(t).copyTo(Ts.getMatRef(i));
    }*/
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
