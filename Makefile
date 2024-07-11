hp:
	@echo " Compile hp_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/hp_main.c ./src/record.c ./src/hp_file.c -lbf -o ./build/hp_main -O2

bf:
	@echo " Compile bf_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bf_main.c ./src/record.c -lbf -o ./build/bf_main -O2;

ht:
	@echo " Compile ht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/record.c ./src/ht_table.c -lbf -o ./build/ht_main -O2

sht:
	@echo " Compile sht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/sht_main.c ./src/record.c ./src/sht_table.c ./src/ht_table.c -lbf -o ./build/sht_main -O2

stats:
	@echo " Compile stats_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/stats_main.c ./src/record.c ./src/sht_table.c ./src/ht_table.c -lbf -o ./build/stats_main -O2

execute_hp: hp
	make clean
	./build/hp_main
execute_ht: ht
	make clean
	./build/ht_main
execute_sht: sht
	make clean
	./build/sht_main

execute_stats: stats
	make clean
	./build/stats_main

clean:
	rm -f data.db index.db