CC=g++
CXXFLAGS=-std=c++11 -O3
LDFLAGS=`pkg-config --cflags --libs cairomm-1.0`

maskwc: maskwc.cc mask_word_cloud.o
	$(CC) $(CXXFLAGS) -o maskwc maskwc.cc mask_word_cloud.o $(LDFLAGS)
mask_word_cloud.o: mask_word_cloud.cc mask_word_cloud.h
	$(CC) $(CXXFLAGS) -c mask_word_cloud.cc $(LDFLAGS)
clean:
	rm -f maskwc *.o

