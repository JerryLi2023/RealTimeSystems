/*
 * SampleCode_SystemInfo.c
 * Source: Lab1_Task1A.c (user-supplied lab example).
 * Purpose: Read CPU count, process ID and hostname.
 * Adaptation: Extracted the useful calls; added missing headers and hostname termination.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/syspage.h>

/* Returns 0 on success, -1 on error (errno is set by gethostname). */
int PrintSystemInfo(void)
{
    char hostnm[100] = {0};
    if (gethostname(hostnm, sizeof(hostnm)) == -1)
        return -1;
    hostnm[sizeof(hostnm) - 1] = '\0';
    printf("CPUs: %d\nPID: %ld\nHostname: %s\n",
           _syspage_ptr->num_cpu, (long)getpid(), hostnm);
    return 0;
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    if (PrintSystemInfo() == -1) { perror("PrintSystemInfo"); return 1; }
    return 0;
}
#endif
