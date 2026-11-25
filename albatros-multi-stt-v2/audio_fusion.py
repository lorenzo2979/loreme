"""Observation adding: align waveforms, then mix in original observation.
This is experimental, not an ASR quality guarantee. No reference text is used.
"""
import numpy as np
from scipy.signal import correlate, correlation_lags
from audio_filter_bank import samples,output

RATIOS=(.25,.5,.75)

def aligned(source,filtered):
    x=samples(source); y=samples(filtered)
    if len(x)!=len(y): raise ValueError("Different input lengths")
    # Global correlation estimates residual delay; bounded to +/-50 ms.
    # Use first differences to reduce bias from DC and low-frequency hum.
    a=np.diff(x); b=np.diff(y)
    scores=correlate(b,a,mode="full",method="fft")
    lags=correlation_lags(len(b),len(a),mode="full")
    mask=np.abs(lags)<=800
    lag=int(lags[mask][np.argmax(scores[mask])])
    if lag>0:
        u=x[:-lag]; v=y[lag:]
    elif lag<0:
        u=x[-lag:]; v=y[:lag]
    else:
        u=x; v=y
    den=float(np.linalg.norm(u)*np.linalg.norm(v))
    confidence=float(np.dot(u,v)/max(den,1e-10))
    if confidence<.25:
        return None,lag,confidence
    # Fill any unavailable edges from the ORIGINAL rather than inventing audio.
    z=x.copy()
    if lag>0: z[:-lag]=y[lag:]
    elif lag<0: z[-lag:]=y[:lag]
    else: z=y.copy()
    return z,lag,confidence

def mix_candidates(source,filtered):
    x=samples(source)
    z,lag,confidence=aligned(source,filtered)
    if z is None:
        return {},{"lag":lag,"correlation":confidence,"status":"alignment_rejected"}
    return {f"mix{int(r*100)}":output(source,(1-r)*x+r*z) for r in RATIOS},{"lag":lag,"correlation":confidence,"status":"aligned"}
