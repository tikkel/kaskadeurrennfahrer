#ifndef _WAVEFUNCTIONS
#define _WAVEFUNCTIONS

extern IDirectSoundBuffer8 *MakeSoundBuffer(IDirectSound8 *ds, LPCWSTR lpSampleName);
extern void *GetWAVRes(LPCWSTR lpResName);
extern BOOL WriteWAVData(IDirectSoundBuffer8 *pDSB, BYTE *pbWaveData, DWORD cbWaveSize);
extern BOOL UnpackWAVChunk(void *pRIFFBytes, LPWAVEFORMATEX *lpwfmx, BYTE **ppbWaveData, DWORD *pcbWaveSize);

#endif /* _WAVEFUNCTIONS */