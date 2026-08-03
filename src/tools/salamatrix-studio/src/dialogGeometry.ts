// Windows dialog units are based on the 8 pt "MS Shell Dlg" font used by the
// native preview template. At 96 DPI its average character cell is 6 x 13 px,
// so one horizontal DLU is 6/4 px and one vertical DLU is 13/8 px.
export const dialogUnitScaleX = 1.5;
export const dialogUnitScaleY = 1.625;
export const designerTitleBarHeight = 29;
export const designerFrameBorder = 1;

export function dialogClientPixels(width: number, height: number): { width: number; height: number } {
  return {
    width: Math.round(width * dialogUnitScaleX),
    height: Math.round(height * dialogUnitScaleY),
  };
}

export function dialogFramePixels(width: number, height: number): { width: number; height: number } {
  const client = dialogClientPixels(width, height);
  return {
    // Keep the existing horizontal footprint, which already matches the native
    // preview. The CSS frame uses border-box sizing; its vertical border must be
    // added so the client region retains the full DLU-derived height.
    width: client.width,
    height: client.height + designerTitleBarHeight + designerFrameBorder * 2,
  };
}
