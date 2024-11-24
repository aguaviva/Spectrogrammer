#ifdef __cplusplus
extern "C" {
#endif

void Spectrogrammer_Init(void *window);
void Spectrogrammer_Shutdown();
bool Spectrogrammer_MainLoopStep();

#ifdef __cplusplus
}
#endif