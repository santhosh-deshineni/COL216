compile: 5stage.cpp 5stage_bypass.cpp 79stage.cpp 79stage_bypass.cpp
		g++ 5stage.cpp -o 5stage
		g++ 5stage_bypass.cpp -o 5stage_bypass
		g++ 79stage.cpp -o 79stage
		g++ 79stage_bypass.cpp -o 79stage_bypass

run_5stage: 5stage input.asm
	./5stage input.asm

run_5stage_bypass: 5stage_bypass input.asm
	./5stage_bypass input.asm

run_79stage: 79stage input.asm
	./79stage input.asm

run_79stage_bypass: 79stage_bypass input.asm
	./79stage_bypass input.asm

clean:
	rm 5stage 5stage_bypass 79stage 79stage_bypass