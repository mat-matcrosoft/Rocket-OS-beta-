# Сборка ROCKET-OS в Windows и запуск в v86

Эта инструкция рассчитана на Windows 10/11. Самый надёжный способ собрать текущую версию — WSL2 + Ubuntu. Нативный MinGW не рекомендуется: проект собирает 32-битный ELF kernel и использует ELF linker script, а обычный Windows GCC обычно выдаёт PE/COFF.

## 1. Установить WSL2

Открой PowerShell от имени администратора:

    wsl --install -d Ubuntu

Перезагрузи Windows, создай пользователя Ubuntu и открой приложение Ubuntu. Все следующие команды выполняй именно в терминале Ubuntu/WSL, а не в обычном PowerShell.

## 2. Установить инструменты

В Ubuntu/WSL:

    sudo apt update
    sudo apt install -y build-essential gcc-multilib binutils nasm qemu-system-x86 xorriso git

Проверь инструменты:

    gcc --version
    nasm -version
    make --version
    xorriso -version

## 3. Скачать проект

Рекомендуется хранить исходники внутри файловой системы WSL:

    cd ~
    git clone https://github.com/mat-matcrosoft/Rocket-OS-beta-.git
    cd Rocket-OS-beta-

Если репозиторий уже лежит на диске Windows, его можно открыть так:

    cd /mnt/c/Users/ТВОЁ_ИМЯ/путь/к/Rocket-OS-beta-

## 4. Запустить тесты

    make clean
    make HOST_CC=gcc test

Ожидаемый результат:

    fat32 tests: ok
    device/driver tests: ok
    shell tests: ok

## 5. Собрать raw-диск

Текущий boot path собирается в 32-битный ELF через обычный GCC WSL:

    make AS=nasm CC=gcc LD=ld OBJCOPY=objcopy image

Результат:

    build/rocket-os.img

Это raw hard-disk image, а не ISO. Для QEMU его подключают как жёсткий диск:

    qemu-system-i386 -drive format=raw,file=build/rocket-os.img

## 6. Собрать ISO

    make AS=nasm CC=gcc LD=ld OBJCOPY=objcopy XORRISO=xorriso iso

Результат:

    build/rocket-os-v1.0.0-prebeta.iso

ISO упаковывает текущий raw boot image как BIOS El Torito hard-disk emulation. Для QEMU:

    qemu-system-i386 -cdrom build/rocket-os-v1.0.0-prebeta.iso -boot d

## 7. Запуск в браузерной v86

Открой: https://copy.sh/v86/

В интерфейсе custom settings выбери правильный тип носителя:

- файл с расширением .iso — поле CD image / CD-диск;
- файл build/rocket-os.img — поле Hard disk image / жёсткий диск;
- поле Floppy disk image / дискета не используй.

Для скачанного prerelease используй rocket-os-v1.0.0-prebeta.iso и выбери именно CD image. После выбора запусти эмулятор или нажми Reset, если он уже был открыт.

Если v86 показывает выбор boot device, выбери CD-ROM. Если используешь raw .img, выбирай hard disk и загружайся с hard disk.

## 8. Запуск локальной копии v86

Если нужен локальный v86, скачай его отдельно:

    git clone https://github.com/copy/v86.git
    cd v86
    python3 -m http.server 8000

Открой http://localhost:8000/ и настрой custom emulator с параметром cdrom, указывающим на ISO. BIOS и VGA BIOS используй из комплекта v86. Браузер может блокировать загрузку локального файла напрямую, поэтому HTTP-сервер предпочтительнее.

## 9. Что нормально увидеть

После загрузки ожидается синий VGA-экран ROCKET-OS и shell prompt:

    rocket>

На текущем образе также может появиться сообщение FAT32: no valid volume on ata0. Это ожидаемо: текущий BIOS Stage 0 загружает kernel как contiguous payload, а FAT32 Stage 2 ещё не является частью boot image.

## 10. Частые ошибки

**missing separator в make:** открой проект в WSL и не запускай Makefile через PowerShell. В Makefile рецепты используют TAB.

**i686-elf-gcc not found:** в WSL используй явные параметры CC=gcc LD=ld OBJCOPY=objcopy; для GCC нужен пакет gcc-multilib.

**No boot device в v86:** ISO был выбран не в CD image, либо выбран raw .img как CD. Для .iso нужен CD-диск.

**BOOT ERROR: DISK READ FAILED:** попробуй ISO из prerelease или подключи build/rocket-os.img как hard disk image. Не подключай raw .img как floppy.

**Нет окна QEMU:** запусти команду из Ubuntu в Windows 11 с WSLg или используй v86 в браузере.

## Ограничения текущего pre-beta

ОС не закончена и содержит ошибки. В текущей версии нет полноценного FAT32 Stage 2, LFN, выделения новых кластеров и полноценной поддержки записи файлов. Используй образ только для тестирования.
