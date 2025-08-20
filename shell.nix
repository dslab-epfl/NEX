{ pkgs ? import (fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/nixos-25.05.tar.gz";
  }) {}
}:

let
  python = pkgs.python312.withPackages (ps: with ps; [
    setuptools
    opencv-python
    numpy
    decorator
    typing-extensions
    scipy
    psutil
    attrs
    packaging
    pytest
    cloudpickle
    cffi
    matplotlib
    mxnet
    tornado
  ]);
in
pkgs.mkShell {
  packages = [
    pkgs.jdk17
    pkgs.sbt
    python
    pkgs.scons
    pkgs.ninja
    pkgs.pkg-config
    pkgs.gcc12
    pkgs.mold-wrapped
    pkgs.glib
    pkgs.libpcap
    pkgs.boost
    pkgs.pixman
    pkgs.nlohmann_json
    pkgs.protobuf
    pkgs.capstone_4
    pkgs.gperftools
    pkgs.git
    pkgs.autoconf
    pkgs.gnumake
    pkgs.gnum4
    pkgs.flex
    pkgs.bison
    pkgs.bc
    pkgs.elfutils
    pkgs.wget
    pkgs.vim
    pkgs.tmux
    pkgs.unzip
    pkgs.perl
    pkgs.rsync
    pkgs.verilator
    pkgs.which
    pkgs.less
    pkgs.util-linux
    pkgs.gmp
    pkgs.libmpc
    pkgs.mpfr
    pkgs.meson
    pkgs.clang_18
    pkgs.jq
    pkgs.zstd
    pkgs.cmake
    pkgs.systemdMinimal
    pkgs.llvmPackages_17.libllvm
    pkgs.libbpf
  ];

  shellHook = ''
    if [ -f .envrc ]; then
      source .envrc
    fi
  '';
}
