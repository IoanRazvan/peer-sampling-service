NPROC=50
compile:
	mpic++ -o sampling-service main.cpp SamplingService.cpp View.cpp
run: compile
	mpirun -np $(NPROC) --oversubscribe ./sampling-service
clean:
	if [ -f sampling-service ]; then \
		rm sampling-service; \
	fi; \