# tasklet.asm - i hate myself

.global enter_tasklet
.type enter_tasklet, @function
enter_tasklet:
	popl %edi
	popl %esi
	jmpl *%esi
