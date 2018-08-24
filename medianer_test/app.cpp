#include <postproc/medianer.h>
#include <GL/freeglut.h>

#define DATASET_OBJ "1529165036844.obj"
#define DATASET_PATH "dataset"

oc::Medianer* medianer;
std::vector<int> noColl;
std::vector<int> coll;

void display(void) {
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

    medianer = new oc::Medianer(DATASET_PATH, DATASET_OBJ);
    for (int i = 0; i <= medianer->GetPoseCount(); i++) {
        if (medianer->RenderTexture(i) > 0) {
            coll.push_back(i);
        } else {
            noColl.push_back(i);
        }
    }
    LOGI("Frames with acceptable collision: %d", noColl.size());
    LOGI("To process: %d", coll.size());
    LOGI("TODO:frames with collision");
    glutMainLoop();
    return 0;
}
