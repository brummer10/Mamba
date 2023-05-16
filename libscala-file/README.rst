``libscala-file``: A Library for Reading Scala Scale Files
==========================================================

About 
-----

``Scala-file`` is a C++ library for reading `scala <http://www.huygens-fokker.org/scala/>`__ 
scale files.  Scala is an Open Source application for working with musical tunings and 
scales.  The scale format has become something of a *de facto* standard for 
describing musical tunings.  This library will read and parse these files. You 
can use it in your C/C++ code to to read these files, should you need to.

Use 
---

The SCL File
.............

The ``.scl`` file encodes the scale's degrees, and can be accessed with the 
``scala::read_scale()`` method.

::

  #include "scala_file.hpp"

  // Get an open file pointer
  ifstream test_scale;
  test_scale.open("scale.scl");

  // Pass it to read_scl to get a scale
  scala::scale loaded_scale = scala::read_scl(test_scale);

The return value is a ``scala::scale``, a small interface wrapping 
a vector of the scale degrees.  They can be queried for the 
ratio of the degree. For example, If we passed a 12-TET scale 
to the following code::

    ifstream test_scale;
    test_scale.open("scales/tet-12.scl");
    scala::scale scale = scala::read_scl(test_scale);

      for (int i = 0; i < scale.get_scale_length(); i++ ){
        cout << "Degree:  "<< i << " Ratio: " << scale.get_ratio(i) << endl;
    };

we would see the following output:

::

  Degree:  0 Ratio: 1
  Degree:  1 Ratio: 1.05946
  Degree:  2 Ratio: 1.12246
  Degree:  3 Ratio: 1.18921
  Degree:  4 Ratio: 1.25992
  Degree:  5 Ratio: 1.33484
  Degree:  6 Ratio: 1.41421
  Degree:  7 Ratio: 1.49831
  Degree:  8 Ratio: 1.5874
  Degree:  9 Ratio: 1.68179
  Degree:  10 Ratio: 1.7818
  Degree:  11 Ratio: 1.88775
  Degree:  12 Ratio: 2

``scala::get_scale_length()`` and ``scala::get_ratio()`` are the two main functions exposed.

The format for the file itself can be found 
`here <http://www.huygens-fokker.org/scala/scl_format.html>`__ .
The specification is done in English, and there are a few ambiguities.  For 
this implementation I:

- Allow blank lines.  The specification only mentions blank lines being allowed 
  in the description. I allow them to be interspersed, so, for example, a blank line 
  at the middle of the file will not cause problems.
- Allow whitespace before the comment character (``!``). Reading through the specification is 
  sounds as if the comment character has to be in the first column.

Given this, this code should parse all valid Scala files, as they are (perhaps) 
a subset of files that this code can parse.

**If this doesn't suit you**: You can define the macro ``SCALA_STRICT`` in ``scala_file.hpp`` 
before compilation to enforce more strict adherence to the standard as detailed.  
If you do this

- Comments must begin in the first column
- Only the description can be blank. If further blank lines are encountered it is 
  assumed to be the end of the file, and parsing stops.
- An inability to parse any line results in an error.

If any issues are encountered a run-time error is thrown (in non-strict mode an 
effort is made to continue when unintelligible lines are encountered).

Note that if you do this the TET-12 test will not pass.

Keyboard Mapping Files 
......................

Scala also defines a keyboard mapping file what maps MIDI note-numbers to 
scale degrees.  These files can be parsed with the ``scala::read_kbm()``
function::

  #include "scala_file.hpp"
  ifstream test_kbm; 
  test_kbm.open("kbm/12-tet.kbm");
  scala::kbm loaded_kbm = scala::read_kbm(test_kbm);

The return value is of type ``scala::kbm``, a light-weight structure capturing 
the values of the input.

::

  struct kbm {
    // Data items
    int map_size;
    int first_note;
    int last_note;
    int middle_note;
    int reference_note;
    double reference_frequency;
    int octave_degree;
    std::vector <int> mapping;
    // Member function...

The File Formats
----------------

The link above gives the actual specifications of the files; this is but a 
brief adumbration.

The ``.scl`` file format itself consists of:

- A short one-line description of the file. Can be blank.
- The number of degrees specified in the file. Integer
- A specification line for each degree. 
- Optional comments, denoted by a ``!`` as the first character of the line. 

**Important Note**. In the Scala scale file the initial degree -- which is 
``1`` by default, is implicit.  In the ``scala::scale`` returned this is 
explicit: ``scale.get_ratio(0)`` will always return 1. 

The degree entries can be decimal numbers (which **must** include a decimal point) 
or as a ratio.  Decimal numbers are interpreted as 
`cents <https://en.wikipedia.org/wiki/Cent_(music)>`__ . A number with a 
slash (``/``) is considered a ratio. A bare number is considered a ratio with
a denominator of one.

Given this, a 12-TET scale could be specified as::

    ! 12-TET
    12 tone equal temperament
    12
    !
    100.0
    200.0
    300.0
    400.0
    500.0
    600.0
    700.0
    800.0
    900.0
    1000.0 cents <- An optional label. Ignored.
    1100.0
    1200.0 

A Pythagorean scale could be specified::

    ! Just intonation
    Pythagorean 
    12
    !
    2187/2048
    9/8
    32/27
    81/64
    4/3
    729/512
    3/2
    6561/4096
    27/16
    16/9
    243/128
    2/1


The ``.kbm`` file is similar in structure: it contains numerical entries and 
optional comments denoted by ``!`` in the first column.  As an example of this 
format, the following would be suitable for a standard mapping of 12-TET.

::

    ! Scala keyboard mapping file, 12-tet
    !
    ! This keyboard mapping file could be used with a 12 TET tuning 
    ! file to create a totally non-microtonal microtonal system.
    !
    ! Map size. There should be an entry for each at the end.
    12
    ! First input degree to map. These are MIDI note numbers. Internally 
    ! MIDI uses a 0-based numbering scheme, although most of the user 
    ! documentation hides this fact. Scala uses 0-based measures.
    0
    ! Last degree to map. Again, a MIDI note number
    127
    ! MIDI note that corresponds to degree 0 of the mapping. In the documentation this is 
    ! called the "middle" note.  Not exactly sure why, but this convention is followed here
    ! (for example, in the struct returned from read_kbm).
    60
    ! Reference note (MIDI number). For absolute tuning.
    69
    ! Frequency of the reference note. Hertz. Float.
    440.0
    ! Scale degree to be used as an octave. This points to a scale degree 
    ! in the SCL file, and if that entry isn't 2/1, then we have octaves which 
    ! do not double. How xenharmonic!
    12
    ! The mapping itself.  There should be 12 entries, as that's what we've 
    ! said at the top of the file. This should be an integer or an "x"
    ! (the x meaning that the degree isn't mapped). It's a little confusing, but
    ! keep in mind that *these* entries are scale degrees, not MIDI notes like 
    ! most of the entries in this file.
    0
    1
    2
    3
    4
    5
    6
    7
    8
    9
    10
    11

If a degree shouldn't be mapped -- in other words, it will not be  assigned to 
any MIDI note, an ``x`` can be placed in the mapping instead of a degree. So
in the following::

    0
    1
    x
    3
    4
    5

the first, second, and fourth MIDI notes will be mapped (to degrees 0, 1, and 3, 
respectively), but the third note will not.

Please see `this page <http://www.huygens-fokker.org/scala/help.htm#mappings>`__ for
a more detailed explanation of the format.

If ``SCALA_STRICT`` is defined:

- The comment character must be in the first line. Otherwise leading 
  whitespace is allowed.
- The ``x`` used to denote an unmapped note must be lower-case. In lax 
  mode a capital ``X`` can also be used.

Compiling and Installing 
------------------------

This library uses `CMake <https://en.wikipedia.org/wiki/CMake>`__, so you 
will need that to build and compile. The simplest build install 
cycle is probably

::

    mkdir build
    cd build/
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make
    make test
    sudo make install

``-DCMAKE_BUILD_TYPE=Release`` can be defined if you don't want debugging symbols 
in the library and want optimizations performed.

Note in testing: The tests make liberal use of ``assert()`` to check for 
error conditions. Some compilers (such as GCC) will optimize those asserts 
out in Release mode, making all tests evergreen. You should build in Debug mode 
if you're running the testing target.

The test suite will run nine different input files. They should run without issue
(unless you're compiling in strict mode, in which case eight of the tests 
will pass).

By default your system will probably try to install to ``/usr/local``
(unless you're on Windows, which has other conventions),
but this can be changed be defining ``CMAKE_INSTALL_PREFIX``.
A header file will be written to ``include``, a library archive 
written to ``lib``, and documentation to ``share/doc/libscala-file``.
To uninstall simply delete these files.

::

    Install the project...
    -- Install configuration: "Release"
    -- Installing: /usr/local/share/doc/libscala-file/README.rst
    -- Installing: /usr/local/share/doc/libscala-file/LICENSE
    -- Installing: /usr/local/lib/libscala-file.a
    -- Installing: /usr/local/include/scala_file.hpp
