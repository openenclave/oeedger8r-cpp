let
  # Let's choose the most recent nixpkgs release for installing
  # most packages. The URL for nixpkgs tarball can be found from
  # will be installed. The URL for the channel can be found from
  #          https://github.com/NixOS/nixpkgs/releases
  # Nix has a 6 month release cycle and the releases are typically
  # numbered year.03 and year.09.
  # Using tarballs is *much* faster than using git to clone the
  # nixpkgs repo. Additionally, tarballs pin the packages to
  # specific versions.
  # To obtain the sha256 for a tarball do:
  #     nix-prefetch-url --unpack <tar-ball-url>
  # That will fetch the tarball, unpack it and print the sha256.
  pkgs = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/20.09.tar.gz";
    sha256 = "1wg61h4gndm3vcprdcg7rc4s1v3jkm5xd7lw8r2f67w502y94gcy";
  }) {};

  # Some build tools like clang exist as different packages e.g
  # clang_8, clang_9 etc in the same nixpkgs release. So it is
  # trivial to choose the appropriate one: pkgs.clang_8 or pkgs.clang_9.
  # Other packages like clang-format-7 may not be available in the latest
  # nixpkg release. To find out how to install an older version of
  # package, choose the channel and the specify the package name here:
  #    https://lazamar.co.uk/nix-versions/
  # That will bring up a list of older versions of the package.
  # Clicking on the version we want will provide the nix code for
  # installing that specific package.
  # For example, the following lists older revisions of clang-tools:
  #    https://lazamar.co.uk/nix-versions/?channel=nixos-20.03&package=clang-tools
  # The instructions for version 7.0.1 provide us the path to the tarball.
  olderPkgs = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/7ff8a16f0726342f0a25697867d8c1306d4da7b0.tar.gz";
    sha256 = "0gwwwzriq4vv0v468mrx17lrmzr34lcdccmsxdfmw7z85fm8afdl";
  }) {};
in
  # Create development environment for building OE SDK.
  # stdenvNoCC means that GCC (default) ought not be made available.
  pkgs.stdenvNoCC.mkDerivation ({
    name = "oe-build-env";
    buildInputs = [
       # It is good to have git within the shell
       # to work with other repos.
       # version=2.29.2
       pkgs.git
      
       # version=3.18.2
       pkgs.cmake
  
       # Order of package declaration matters.
       # Specify clang-tools first so that the
       # older clang-format (7.0.1) appears before the
       # newer clang-format (8.0.1) that is part of clang_8.
       # version=7.0.1
       olderPkgs.clang-tools

       # OE SDK is built using clang.
       # version=8.0.1
       pkgs.clang_8

       # Use ccache to speed up recompilation.
       # See shellHooks for configuration.
       pkgs.ccache
       
       # Allow using ninja instead of make.
       # version=1û10.1
       pkgs.ninja

       # OpenSSL tests require. Perl.
       # The specific version may not matter.
       # version=5.32.0
       pkgs.perl

       # Python is used in many places in the SDK.
       # See shellHooks for how cmake-format is setup.
       # version=3.8.5
       pkgs.python3

       # Recent version of openssl is used.
       # This may need to be selectively updated as newer
       # versions are released.
       # version=1.1.1g
       pkgs.openssl

       # Doxygen for SDK document generation.
       # version=1.8.19
       pkgs.doxygen

       # Graphviz is needed for the dot commant used by the
       # document generation.
       # dot version=2.43.0
       pkgs.graphviz
    
       # Needed for debugging the SDK. 
       # version=9.2
       pkgs.gdb

       # Fun
       pkgs.neofetch
    ];
    shellHooks = ''
      # Set up C and C++ compilers explicitly via
      # environment variables. This ensures that
      # these compilers are picked up even if the
      # shell has some other compilers lying around.
      export CC=clang
      export CXX=clang++

      # Set up the base dir so that ccache uses relative
      # paths from the base dir. This will result in
      # cache hits across many local repos.
      export CCACHE_BASEDIR=`pwd`

      # Setup python virtual env and install cmake-format.
      # There might be better ways to install cmake-format in
      # nix, but this is good enough for now.
      rm -rf .venv
      python -m venv .venv
      source .venv/bin/activate
      # Send "upgrade pip" warning to /dev/null.
      pip install cmake-format==0.6.9 2>/dev/null
    '';
  })
