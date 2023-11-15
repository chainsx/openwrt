# 本仓库基于 ImmortalWrt


## 特别提醒

1. 本仓库有引用自 [Lean's lede](https://github.com/coolsnowwolf/lede) 的相关内容，引用请注明。
2. 本仓库有引用自 [immortalwrt](https://github.com/immortalwrt/immortalwrt) 的相关内容，引用请注明。
3. 本仓库有引用自 [Armbian Linux](https://github.com/armbian/build)` 的相关内容，引用请注明。

## 注意

1. **不要用 root 用户进行编译**
2. 默认登陆IP 192.168.1.1 密码 password

## 损坏的包

```
pdnsd-alt 1.2.9b-par
```

## 编译命令

1. 首先装好 Linux 系统，推荐 Debian 11 或 Ubuntu LTS

2. 安装编译依赖

   ```bash
   sudo apt update -y
   sudo apt full-upgrade -y
   sudo apt install -y ack antlr3 asciidoc autoconf automake autopoint binutils bison build-essential \
   bzip2 ccache cmake cpio curl device-tree-compiler fastjar flex gawk gettext gcc-multilib g++-multilib \
   git gperf haveged help2man intltool libc6-dev-i386 libelf-dev libglib2.0-dev libgmp3-dev libltdl-dev \
   libmpc-dev libmpfr-dev libncurses5-dev libncursesw5-dev libreadline-dev libssl-dev libtool lrzsz \
   mkisofs msmtp nano ninja-build p7zip p7zip-full patch pkgconf python2.7 python3 python3-pyelftools \
   libpython3-dev qemu-utils rsync scons squashfs-tools subversion swig texinfo uglifyjs upx-ucl unzip \
   vim wget xmlto xxd zlib1g-dev
   ```

3. 下载源代码，更新 feeds 并选择配置

   ```bash
   git clone https://github.com/chainsx/openwrt
   cd lede
   ./scripts/feeds update -a
   ./scripts/feeds install -a
   make menuconfig
   ```

4. 下载 dl 库，编译固件
（-j 后面是线程数，第一次编译推荐用单线程）

   ```bash
   make download -j8
   make V=s -j1
   ```

！！！ 本套代码不保证肯定可以编译成功。

你可以自由使用，但源码编译二次发布请注明 GitHub 仓库链接！

二次编译：

```bash
cd lede
git pull
./scripts/feeds update -a
./scripts/feeds install -a
make defconfig
make download -j8
make V=s -j$(nproc)
```

如果需要重新配置：

```bash
rm -rf ./tmp && rm -rf .config
make menuconfig
make V=s -j$(nproc)
```

编译完成后输出路径：bin/targets

