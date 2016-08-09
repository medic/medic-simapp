/**
 * Muvuku: An STK data collection framework
 *
 * Copyright 2011-2012 Medic Mobile, Inc. <hello@medicmobile.org>
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL MEDIC MOBILE BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MUVUKU_FLASH_H__
#define __MUVUKU_FLASH_H__

#include "bladox.h"
#include "prototype.h"


/* Flash memory to reserve:
    This parameter determines the amount of storage to reserve
    for Muvuku's flash memory pool driver. Note that this increases
    the size of the resulting binary, and maxes out at about 30KiB. */

#ifndef MUVUKU_FLASH_RESERVED
    #define MUVUKU_FLASH_RESERVED 2048 /* bytes */
#endif /* MUVUKU_FLASH_RESERVED */


/* Reserved flash memory:
    This is used by Muvuku's flash memory (pool) driver. */

extern u8 PROGMEM muvuku_flash_reserved[MUVUKU_FLASH_RESERVED];


#endif /* __MUVUKU_FLASH_H__ */

