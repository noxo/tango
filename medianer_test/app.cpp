#include <postproc/medianer.h>
#include <GL/freeglut.h>

#define DATASET_OBJ "1529165036844.obj"
#define DATASET_PATH "dataset"
#define DATASET_PREPROCESS false

unsigned int idx = 0;
oc::Medianer* medianer;
std::vector<int> noColl;
std::vector<int> poses;

void display(void) {
    if (!poses.empty()) {
        if (medianer->RenderTexture(poses[idx])) {
            noColl.push_back(poses[idx]);
            poses.erase(poses.begin() + idx);
        } else {
            idx++;
        }
        if (idx >= poses.size()) {
            idx = 0;
            LOGI("Frames without collision: %d", noColl.size());
            LOGI("To process: %d", poses.size());
            LOGI("TODO:frames with collision");
        }
    }
    medianer->RenderPose(5);

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

    medianer = new oc::Medianer(DATASET_PATH, DATASET_OBJ, DATASET_PREPROCESS);
    for (int i = 0; i <= medianer->GetPoseCount(); i++)
        poses.push_back(i);
    glutMainLoop();
    return 0;
}
