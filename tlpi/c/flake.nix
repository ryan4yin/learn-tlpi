{
  description = "The Linux Programming Interface";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = {nixpkgs, ...}: let
    systems = [
      "x86_64-linux"
      "aarch64-linux"
    ];
    # Helper function to generate a set of attributes for each system
    forAllSystems = func: (nixpkgs.lib.genAttrs systems func);
  in {
    devShells = forAllSystems (system: let
      pkgs = import nixpkgs {inherit system;};
      lib = pkgs.lib;
      tlpi-dist = pkgs.callPackage ./pkgs/tlpi.nix {};
    in {
      default = pkgs.mkShell rec {
        # https://github.com/NixOS/nixpkgs/blob/nixos-unstable/doc/stdenv/stdenv.chapter.md#overview-ssec-stdenv-dependencies-overview

        # programs/hooks can be called in dev env
        nativeBuildInputs = with pkgs; [
          gcc
          just
        ];
        # libraries that will end up copied or linked into the final output
        # or programs that are expected to be in the PATH at runtime
        #
        # What it will do:
        #   1. NIX_CFLAGS_COMPILE: add all the dependencies' `include` directories to this env
        #      This env var will be used by nix's CC Wrapper(gcc/clang) to include the headers
        #   
        #   2. NIX_LDFLAGS: add all the dependencies' `lib` & `lib64` directories to this env
        #      This env var will be used by nix's Bintools Wrapper to link the libraries
        buildInputs = with pkgs; [
          tlpi-dist
        ];
        # LD_LIBRARY_PATH = lib.makeLibraryPath buildInputs;
      };
    });

    packages = forAllSystems (system: let
      pkgs = import nixpkgs {inherit system;};
    in {
      tlpi-dist = pkgs.callPackage ./pkgs/tlpi.nix {};
    });
  };
}
