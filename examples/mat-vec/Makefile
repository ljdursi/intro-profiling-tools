all: mvm mvm-omp mvm-omp-profile 

clean: 
	rm -f mvm mvm-omp mvm-omp-profile mvm-profile gmon*out timings*out *.openss Mat-vec.dat

mvm: mat-vec-mult.c
	gcc -g -O3 -o $@ $^ --std=c99 

mvm-omp: mat-vec-mult.c
	gcc -O3 -o $@ -fopenmp $^ --std=c99 -lgomp

mvm-omp-scalasca: mat-vec-mult.c
	scalasca -instrument gcc -O3 -o $@ -fopenmp $^ --std=c99 -lgomp

mvm-omp-profile: mat-vec-mult.c
	gcc -O3 -pg -g -o $@ -fopenmp $^ --std=c99 -lgomp

mvm-profile: mat-vec-mult.c
	gcc -O3 -pg -g -o $@ $^ --std=c99 

ARGS="--matsize=2500"

timings.out: mvm
	./mvm ${ARGS} | tee $@

timings-binary.out: mvm
	./mvm ${ARGS} --binary | tee $@

timings-transpose.out: mvm
	./mvm ${ARGS} --transpose  | tee $@

gmon-8threads.out: mvm-omp-profile
	(OMP_NUM_THREADS=8 ./mvm-omp-profile ${ARGS})
	mv gmon.out $@

gmon-1thread.out: mvm-omp-profile
	(OMP_NUM_THREADS=1 ./mvm-omp-profile ${ARGS})
	mv gmon.out $@

gmon-8threads-transpose.out: mvm-omp-profile
	(OMP_NUM_THREADS=8 ./mvm-omp-profile ${ARGS} --transpose)
	mv gmon.out $@

gmon-1thread-transpose.out: mvm-omp-profile
	(OMP_NUM_THREADS=1 ./mvm-omp-profile ${ARGS} --transpose)
	mv gmon.out $@

mvm-notranspose-pcsamp.openss: mvm
	openss -f "mvm --matsize=2500 --binary" pcsamp
	mv mvm-pcsamp.openss $@

mvm-transpose-pcsamp.openss: mvm
	openss -f "mvm --matsize=2500 --binary --transpose" pcsamp
	mv mvm-pcsamp.openss $@

mvm-notranspose-usertime.openss: mvm
	openss -f "mvm --matsize=2500 --binary" usertime
	mv mvm-usertime.openss $@

mvm-transpose-usertime.openss: mvm
	openss -f "mvm --matsize=2500 --binary --transpose" usertime
