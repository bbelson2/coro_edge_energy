#define PEM_MAX_EVENTS 6 

#ifdef __cplusplus
extern "C" {
#endif

int pem_setup(int which);
int pem_get_descriptor(int i, int* type, int* config, const char** name);

int pem_start(void);
int pem_stop(void);

int pem_read_totals(int count, long long* buffer);

void pem_CollectDataAccessEvents(void*);
void pem_MeasureDataAccessEvents();


#ifdef __cplusplus
} // extern "C"
#endif
