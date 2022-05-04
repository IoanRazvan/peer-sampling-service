NPROC=50
compile:
	mpic++ -o sampling-service src/main.cpp src/SamplingService.cpp src/View.cpp -Iinclude
run: compile
	mpirun -np $(NPROC) --oversubscribe ./sampling-service
clean:
	if [ -f sampling-service ]; then \
		rm sampling-service; \
	fi; \