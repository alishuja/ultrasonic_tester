all: ultrasonic_tester

ultrasonic_tester: ultrasonic_tester.c
	gcc ultrasonic_tester.c -o ultrasonic_tester

clean:
	rm -rf ultrasonic_tester *~
