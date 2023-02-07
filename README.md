# stegng - A steganography tool for png's

## Description
Hide information in PNG images disguised as parts of the image format.
Information may be a simple string or an entire file. Information hidden by
`stegng` can also be extracted from the PNG file.

## Usage
`stegng` expects an input file to work with, `stegng -i file.png`
If no other options are given the program simply performs checks on the file,
mainly if it's a valid PNG file.

## Options

+ `-i <PATH>` specifies the input file
+ `-o <PATH>` specifies the output file, if `-i` and `-o` are the only options
given the program simply copies the input image
+ `-p` tells the program to print the chunks present in the image in a human readable format
+ `-s` remove the non-critical chunks from the image before writing them to the output file
+ `-j <STRING>` injects the string specified by `<STRING>` inside the output image.
It does this by creating the appropriate number of new chunks (`tEXt` by default)
and placing the `<STRING>` specified inside the data field of the new chunks,
splitting the data into multiple chunks if needed.
+ `-J <PATH>` reads the data to be inserted from the file specified by `<PATH>`.
Same rules apply as the `-j` option.
+ `-x <INTEGER>` specifies at which chunk index to insert the new chunks,
negative numbers insert from the end
+ `-d <INTEGER>` specifies the maximum size in bytes of the data field of the
injected chunks before splitting the data in multiple chunks
+ `-e <STRING>` specifies from which chunk to extract the data fields which
possible contain data previously injected by the program, example `stegng -i image.png -e tEXt`
+ `-t <STRING>` tells the program to create and inject the data into chunks of
type `<STRING>`. The type of the new chunk may be a type that is not part of
the PNG standard.

## Examples

```
./stegng -i ./media/maze.png -p # print chunks information
./stegng -i ./media/maze.png -j 'this is a secret' -o ./newimage.png # inject the string 'this is a secret'
./stegng -i ./newimage.png -e tEXt # extract injected string
./stegng -i ./media/maze.png -J ./secret_file.gpg -d 300 -o ./newimage.png # inject the file secret_file.gpg, setting the maximum chunk size at 300 bytes
./stegng -i ./newimage.png -e tEXt > secret_file.gpg # extract injected data from tEXt chunk
```

## Installation
```
git clone https://github.com/begone-prop/stegng.git
cd stegng
make
```
