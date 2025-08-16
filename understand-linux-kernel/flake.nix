{
  description = "shell for linux kernel development";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.05";
  };

  outputs =
    { nixpkgs, ... }:
    let
      pkgs = import nixpkgs { };
    in
    {
      # use `nix develop .#kernel` to enter the environment with the custom kernel build environment available.
      # and then use `unpackPhase` to unpack the kernel source code and cd into it.
      # then you can use `make menuconfig` to configure the kernel.
      #
      # problem
      #   - using `make menuconfig` - Unable to find the ncurses package.
      devShells.x86_64-linux.kernel = pkgs.linuxPackages_latest.kernel.dev;

      # use `nix develop .#fhs` to enter the fhs test environment defined here.
      devShells.x86_64-linux.fhs =
        # the code here is mainly copied from:
        #   https://wiki.nixos.org/wiki/Linux_kernel#Embedded_Linux_Cross-compile_xconfig_and_menuconfig
        (pkgs.buildFHSUserEnv {
          name = "kernel-build-env";
          targetPkgs =
            pkgs_:
            (
              with pkgs_;
              [
                linuxPackages.kernel.dev
                linuxPackages.bcc # eBPF tools
                qemu
                ccache
                bc
                bison
                flex
                libelf
                libnl
                openssl
                perl
                python3
                rsync
                swig
                zstd
                gdb
                strace
                perf-tools
                bpftrace

                # we need theses packages to run `make menuconfig` successfully.
                pkgconfig
                ncurses

                gcc
              ]
              ++ pkgs.linux.nativeBuildInputs
            );
          runScript = pkgs.writeScript "init.sh" ''
            export PKG_CONFIG_PATH="${pkgs.ncurses.dev}/lib/pkgconfig:"
            exec bash
          '';
        }).env;
    };
}
