#include <postproc/medianer.h>
#include <GL/freeglut.h>

#define DATASET_OBJ "1529165036844.obj"
#define DATASET_PATH "dataset"
#define DATASET_PREPROCESS false

oc::Medianer* medianer;
std::vector<int> noColl;
std::vector<int> coll;
int idx = 0;
float lastError;
float mod[9];
int modIndex;
int modPhase;

void display(void) {
    float step = 0;
    switch (modIndex / 9) {
        case(0): step = 0.05f; break;
        case(1): step = 0.01f; break;
    }
    if (modPhase == 0) {
        mod[modIndex % 9] += step;
    } else if (modPhase == 1) {
        mod[modIndex % 9] -= step;
    }
    medianer->SetModification(glm::vec3(mod[0], mod[1], mod[2]),
            glm::vec3(1) - glm::vec3(mod[3], mod[4], mod[5]),
            glm::vec3(mod[6], mod[7], mod[8]));

    float error = medianer->RenderTexture(coll[idx]);
    if ((error > lastError) || (error == 0)) {
        if (modPhase == 0) {
            mod[modIndex % 9] -= step;
        } else if (modPhase == 1) {
            mod[modIndex % 9] += step;
        }
        modPhase++;
        if (modPhase == 2) {
            modIndex++;
            modPhase = 0;
        }
        if (modIndex == 18) {
            //TODO:store modification
            idx++;
            if (idx == coll.size())
                std::exit(0);
            lastError = 100;
            modIndex = 0;
            modPhase = 0;
        }
    } else {
        lastError = error;
        LOGI("Error: %.3f", lastError);
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
    for (int i = 0; i <= medianer->GetPoseCount(); i++) {
        if (medianer->RenderTexture(i) > 0) {
            coll.push_back(i);
        } else {
            noColl.push_back(i);
        }
    }
    lastError = 100;
    for (modIndex = 0; modIndex < 9; modIndex++) {
        mod[modIndex] = 0;
    }
    modIndex = 0;
    modPhase = 0;
    LOGI("Frames with acceptable collision: %d", noColl.size());
    LOGI("To process: %d", coll.size());
    LOGI("TODO:frames with collision");
    glutMainLoop();
    return 0;
}
