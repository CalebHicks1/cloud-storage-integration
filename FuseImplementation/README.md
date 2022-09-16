FUSE poc using local files executes a program getFile that utilizes the jansson json formatter library. In order to run this with the getFile program run the following commands


gunzip -c jansson-2.13.tar.gz | tar xf -
cd jansson-2.13
./configure
make
make check
sudo make install


return to the SSFS directory and run 
make
./fuse -f fusemtpt

In a new terminal window 

cd fusemtpt 
ls
You should be able to see two empty files.


Before pushing, run:
make clean
make cleanJan

*Once make cleanJan is run you will have to re unzip the jansson library