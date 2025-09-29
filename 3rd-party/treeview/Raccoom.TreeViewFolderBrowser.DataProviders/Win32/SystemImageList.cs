// Copyright © 2009 by Christoph Richner. All rights are reserved.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 
// website http://www.raccoom.net, email support@raccoom.net, msn chrisdarebell@msn.com

namespace Raccoom.Win32
{

	#region Public Enumerations

	/// <summary>
	/// Available system image list sizes
	/// </summary>
	public enum SystemImageListSize : int
	{
		/// <summary>
		/// System Large Icon Size (typically 32x32)
		/// </summary>
		LargeIcons = 0x0,
		/// <summary>
		/// System Small Icon Size (typically 16x16)
		/// </summary>
		SmallIcons = 0x1,
		/// <summary>
		/// System Extra Large Icon Size (typically 48x48).
		/// Only available under XP; under other OS the
		/// Large Icon ImageList is returned.
		/// </summary>
		ExtraLargeIcons = 0x2
	}

	/// <summary>
	/// Flags controlling how the Image List item is 
	/// drawn
	/// </summary>
	[System.Flags]
	public enum ImageListDrawItemConstants : int
	{
		/// <summary>
		/// Draw item normally.
		/// </summary>
		ILD_NORMAL = 0x0,
		/// <summary>
		/// Draw item transparently.
		/// </summary>
		ILD_TRANSPARENT = 0x1,
		/// <summary>
		/// Draw item blended with 25% of the specified foreground colour
		/// or the Highlight colour if no foreground colour specified.
		/// </summary>
		ILD_BLEND25 = 0x2,
		/// <summary>
		/// Draw item blended with 50% of the specified foreground colour
		/// or the Highlight colour if no foreground colour specified.
		/// </summary>
		ILD_SELECTED = 0x4,
		/// <summary>
		/// Draw the icon's mask
		/// </summary>
		ILD_MASK = 0x10,
		/// <summary>
		/// Draw the icon image without using the mask
		/// </summary>
		ILD_IMAGE = 0x20,
		/// <summary>
		/// Draw the icon using the ROP specified.
		/// </summary>
		ILD_ROP = 0x40,
		/// <summary>
		/// Preserves the alpha channel in dest. XP only.
		/// </summary>
		ILD_PRESERVEALPHA = 0x1000,
		/// <summary>
		/// Scale the image to cx, cy instead of clipping it.  XP only.
		/// </summary>
		ILD_SCALE = 0x2000,
		/// <summary>
		/// Scale the image to the current DPI of the display. XP only.
		/// </summary>
		ILD_DPISCALE = 0x4000
	}

	/// <summary>
	/// Enumeration containing XP ImageList Draw State options
	/// </summary>
	[System.Flags]
	public enum ImageListDrawStateConstants : int
	{
		/// <summary>
		/// The image state is not modified. 
		/// </summary>
		ILS_NORMAL = (0x00000000),
		/// <summary>
		/// Adds a glow effect to the icon, which causes the icon to appear to glow 
		/// with a given color around the edges. (Note: does not appear to be
		/// implemented)
		/// </summary>
		ILS_GLOW = (0x00000001), //The color for the glow effect is passed to the IImageList::Draw method in the crEffect member of IMAGELISTDRAWPARAMS. 
		/// <summary>
		/// Adds a drop shadow effect to the icon. (Note: does not appear to be
		/// implemented)
		/// </summary>
		ILS_SHADOW = (0x00000002), //The color for the drop shadow effect is passed to the IImageList::Draw method in the crEffect member of IMAGELISTDRAWPARAMS. 
		/// <summary>
		/// Saturates the icon by increasing each color component 
		/// of the RGB triplet for each pixel in the icon. (Note: only ever appears
		/// to result in a completely unsaturated icon)
		/// </summary>
		ILS_SATURATE = (0x00000004), // The amount to increase is indicated by the frame member in the IMAGELISTDRAWPARAMS method. 
		/// <summary>
		/// Alpha blends the icon. Alpha blending controls the transparency 
		/// level of an icon, according to the value of its alpha channel. 
		/// (Note: does not appear to be implemented).
		/// </summary>
		ILS_ALPHA = (0x00000008) //The value of the alpha channel is indicated by the frame member in the IMAGELISTDRAWPARAMS method. The alpha channel can be from 0 to 255, with 0 being completely transparent, and 255 being completely opaque. 
	}

	/// <summary>
	/// Flags specifying the state of the icon to draw from the Shell
	/// </summary>
	[System.Flags]
	public enum ShellIconStateConstants
	{
		/// <summary>
		/// Get icon in normal state
		/// </summary>
		ShellIconStateNormal = 0,
		/// <summary>
		/// Put a link overlay on icon 
		/// </summary>
		ShellIconStateLinkOverlay = 0x8000,
		/// <summary>
		/// show icon in selected state 
		/// </summary>
		ShellIconStateSelected = 0x10000,
		/// <summary>
		/// get open icon 
		/// </summary>
		ShellIconStateOpen = 0x2,
		/// <summary>
		/// apply the appropriate overlays
		/// </summary>
		ShellIconAddOverlays = 0x000000020,
	}

	#endregion

	#region SystemImageList class

	/// <summary>
	/// Sys32ImageList wrapper class
	/// </summary>
	public class SystemImageList : System.IDisposable
	{
		#region Constants

		private const int MAX_PATH = 260;

		private const int FILE_ATTRIBUTE_NORMAL = 0x80;
		private const int FILE_ATTRIBUTE_DIRECTORY = 0x10;

		private const int FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x100;
		private const int FORMAT_MESSAGE_ARGUMENT_ARRAY = 0x2000;
		private const int FORMAT_MESSAGE_FROM_HMODULE = 0x800;
		private const int FORMAT_MESSAGE_FROM_STRING = 0x400;
		private const int FORMAT_MESSAGE_FROM_SYSTEM = 0x1000;
		private const int FORMAT_MESSAGE_IGNORE_INSERTS = 0x200;
		private const int FORMAT_MESSAGE_MAX_WIDTH_MASK = 0xFF;

		#endregion

		#region Fields

		private System.IntPtr hIml = System.IntPtr.Zero;
		private IImageList iImageList = null;
		private SystemImageListSize size = SystemImageListSize.SmallIcons;
		private bool disposed = false;

		#endregion

		#region Private Enumerations

		[System.Flags]
		private enum SHGetFileInfoConstants : int
		{
			SHGFI_ICON = 0x100, // get icon 
			SHGFI_DISPLAYNAME = 0x200, // get display name 
			SHGFI_TYPENAME = 0x400, // get type name 
			SHGFI_ATTRIBUTES = 0x800, // get attributes 
			SHGFI_ICONLOCATION = 0x1000, // get icon location 
			SHGFI_EXETYPE = 0x2000, // return exe type 
			SHGFI_SYSICONINDEX = 0x4000, // get system icon index 
			SHGFI_LINKOVERLAY = 0x8000, // put a link overlay on icon 
			SHGFI_SELECTED = 0x10000, // show icon in selected state 
			SHGFI_ATTR_SPECIFIED = 0x20000, // get only specified attributes 
			SHGFI_LARGEICON = 0x0, // get large icon 
			SHGFI_SMALLICON = 0x1, // get small icon 
			SHGFI_OPENICON = 0x2, // get open icon 
			SHGFI_SHELLICONSIZE = 0x4, // get shell size icon 
			//SHGFI_PIDL = 0x8,                  // pszPath is a pidl 
			SHGFI_USEFILEATTRIBUTES = 0x10, // use passed dwFileAttribute 
			SHGFI_ADDOVERLAYS = 0x000000020, // apply the appropriate overlays
			SHGFI_OVERLAYINDEX = 0x000000040 // Get the index of the overlay
		}

		#endregion

		#region Private ImageList structures

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		private struct RECT
		{
			int left;
			int top;
			int right;
			int bottom;
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		private struct POINT
		{
			int x;
			int y;
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		private struct IMAGELISTDRAWPARAMS
		{
			public int cbSize;
			public System.IntPtr himl;
			public int i;
			public System.IntPtr hdcDst;
			public int x;
			public int y;
			public int cx;
			public int cy;
			public int xBitmap; // x offest from the upperleft of bitmap
			public int yBitmap; // y offset from the upperleft of bitmap
			public int rgbBk;
			public int rgbFg;
			public int fStyle;
			public int dwRop;
			public int fState;
			public int Frame;
			public int crEffect;
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		private struct IMAGEINFO
		{
			public System.IntPtr hbmImage;
			public System.IntPtr hbmMask;
			public int Unused1;
			public int Unused2;
			public RECT rcImage;
		}

		[System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
		private struct SHFILEINFO
		{
			public System.IntPtr hIcon;
			public int iIcon;
			public int dwAttributes;
			[System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst=MAX_PATH)] public string szDisplayName;
			[System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst=80)] public string szTypeName;
		}

		#endregion

		#region Constructors, Dispose, Destructor

		/// <summary>
		/// Creates a Small Icons SystemImageList 
		/// </summary>
		public SystemImageList()
		{
			create();
		}

		/// <summary>
		/// Creates a SystemImageList with the specified size
		/// </summary>
		/// <param name="size">Size of System ImageList</param>
		public SystemImageList(SystemImageListSize size)
		{
			this.size = size;
			create();
		}

		/// <summary>
		/// Clears up any resources associated with the SystemImageList
		/// </summary>
		public void Dispose()
		{
			Dispose(true);
			System.GC.SuppressFinalize(this);
		}

		/// <summary>
		/// Clears up any resources associated with the SystemImageList
		/// when disposing is true.
		/// </summary>
		/// <param name="disposing">Whether the object is being disposed</param>
		public virtual void Dispose(bool disposing)
		{
			if (!disposed)
			{
				if (disposing)
				{
					if (iImageList != null)
					{
						System.Runtime.InteropServices.Marshal.ReleaseComObject(iImageList);
					}
					iImageList = null;
				}
			}
			disposed = true;
		}

		/// <summary>
		/// Finalise for SysImageList
		/// </summary>
		~SystemImageList()
		{
			Dispose(false);
		}

		#endregion

		#region Implementation

		[System.Runtime.InteropServices.DllImport("shell32")]
		private static extern System.IntPtr SHGetFileInfo(
			string pszPath,
			int dwFileAttributes,
			ref SHFILEINFO psfi,
			uint cbFileInfo,
			uint uFlags);

		[System.Runtime.InteropServices.DllImport("shell32")]
		private static extern System.IntPtr SHGetFileInfo(
			System.IntPtr pid,
			int dwFileAttributes,
			ref SHFILEINFO psfi,
			uint cbFileInfo,
			uint uFlags);

		[System.Runtime.InteropServices.DllImport("user32.dll")]
		private static extern int DestroyIcon(System.IntPtr hIcon);

		[System.Runtime.InteropServices.DllImport("kernel32")]
		private extern static int FormatMessage(
			int dwFlags,
			System.IntPtr lpSource,
			int dwMessageId,
			int dwLanguageId,
			string lpBuffer,
			uint nSize,
			int argumentsLong);

		[System.Runtime.InteropServices.DllImport("kernel32")]
		private extern static int GetLastError();

		[System.Runtime.InteropServices.DllImport("comctl32")]
		private extern static int ImageList_Draw(
			System.IntPtr hIml,
			int i,
			System.IntPtr hdcDst,
			int x,
			int y,
			int fStyle);

		[System.Runtime.InteropServices.DllImport("comctl32")]
		private extern static int ImageList_DrawIndirect(
			ref IMAGELISTDRAWPARAMS pimldp);

		[System.Runtime.InteropServices.DllImport("comctl32")]
		private extern static int ImageList_GetIconSize(
			System.IntPtr himl,
			ref int cx,
			ref int cy);

		[System.Runtime.InteropServices.DllImport("comctl32")]
		private extern static System.IntPtr ImageList_GetIcon(
			System.IntPtr himl,
			int i,
			int flags);

		/// <summary>
		/// SHGetImageList is not exported correctly in XP.  See KB316931
		/// http://support.microsoft.com/default.aspx?scid=kb;EN-US;Q316931
		/// Apparently (and hopefully) ordinal 727 isn't going to change.
		/// </summary>
		[System.Runtime.InteropServices.DllImport("shell32.dll", EntryPoint = "#727")]
		private extern static int SHGetImageList(
			int iImageList,
			ref System.Guid riid,
			ref IImageList ppv
			);

		[System.Runtime.InteropServices.DllImport("shell32.dll", EntryPoint = "#727")]
		private extern static int SHGetImageListHandle(
			int iImageList,
			ref System.Guid riid,
			ref System.IntPtr handle
			);

		/// <summary>
		/// Determines if the system is running Windows XP
		/// or above
		/// </summary>
		/// <returns>True if system is running XP or above, False otherwise</returns>
		private bool isXpOrAbove()
		{
			bool ret = false;
			if (System.Environment.OSVersion.Version.Major > 5)
			{
				ret = true;
			}
			else if ((System.Environment.OSVersion.Version.Major == 5) &&
				(System.Environment.OSVersion.Version.Minor >= 1))
			{
				ret = true;
			}
			return ret;
			//return false;
		}

		/// <summary>
		/// Creates the SystemImageList
		/// </summary>
		private void create()
		{
			// forget last image list if any:
			hIml = System.IntPtr.Zero;

			if (isXpOrAbove())
			{
				// Get the System IImageList object from the Shell:
				System.Guid iidImageList = new System.Guid("46EB5926-582E-4017-9FDF-E8998DAA0950");
				int ret = SHGetImageList(
					(int) size,
					ref iidImageList,
					ref iImageList
					);
				// the image list handle is the IUnknown pointer, but 
				// using Marshal.GetIUnknownForObject doesn't return
				// the right value.  It really doesn't hurt to make
				// a second call to get the handle:
				SHGetImageListHandle((int) size, ref iidImageList, ref hIml);
			}
			else
			{
				// Prepare flags:
				SHGetFileInfoConstants dwFlags = SHGetFileInfoConstants.SHGFI_USEFILEATTRIBUTES | SHGetFileInfoConstants.SHGFI_SYSICONINDEX;
				if (size == SystemImageListSize.SmallIcons)
				{
					dwFlags |= SHGetFileInfoConstants.SHGFI_SMALLICON;
				}
				// Get image list
				SHFILEINFO shfi = new SHFILEINFO();
				uint shfiSize = (uint) System.Runtime.InteropServices.Marshal.SizeOf(shfi.GetType());

				// Call SHGetFileInfo to get the image list handle
				// using an arbitrary file:
				hIml = SHGetFileInfo(
					".txt",
					FILE_ATTRIBUTE_NORMAL,
					ref shfi,
					shfiSize,
					(uint) dwFlags);
				System.Diagnostics.Debug.Assert((hIml != System.IntPtr.Zero), "Failed to create Image List");
			}
		}

		#region Private ImageList COM Interop (XP)

		[System.Runtime.InteropServices.ComImport()]
		[System.Runtime.InteropServices.Guid("46EB5926-582E-4017-9FDF-E8998DAA0950")]
		[System.Runtime.InteropServices.InterfaceType(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
		//helpstring("Image List"),
			interface IImageList
		{
			[System.Runtime.InteropServices.PreserveSig]
			int Add(
				System.IntPtr hbmImage,
				System.IntPtr hbmMask,
				ref int pi);

			[System.Runtime.InteropServices.PreserveSig]
			int ReplaceIcon(
				int i,
				System.IntPtr hicon,
				ref int pi);

			[System.Runtime.InteropServices.PreserveSig]
			int SetOverlayImage(
				int iImage,
				int iOverlay);

			[System.Runtime.InteropServices.PreserveSig]
			int Replace(
				int i,
				System.IntPtr hbmImage,
				System.IntPtr hbmMask);

			[System.Runtime.InteropServices.PreserveSig]
			int AddMasked(
				System.IntPtr hbmImage,
				int crMask,
				ref int pi);

			[System.Runtime.InteropServices.PreserveSig]
			int Draw(
				ref IMAGELISTDRAWPARAMS pimldp);

			[System.Runtime.InteropServices.PreserveSig]
			int Remove(
				int i);

			[System.Runtime.InteropServices.PreserveSig]
			int GetIcon(
				int i,
				int flags,
				ref System.IntPtr picon);

			[System.Runtime.InteropServices.PreserveSig]
			int GetImageInfo(
				int i,
				ref IMAGEINFO pImageInfo);

			[System.Runtime.InteropServices.PreserveSig]
			int Copy(
				int iDst,
				IImageList punkSrc,
				int iSrc,
				int uFlags);

			[System.Runtime.InteropServices.PreserveSig]
			int Merge(
				int i1,
				IImageList punk2,
				int i2,
				int dx,
				int dy,
				ref System.Guid riid,
				ref System.IntPtr ppv);

			[System.Runtime.InteropServices.PreserveSig]
			int Clone(
				ref System.Guid riid,
				ref System.IntPtr ppv);

			[System.Runtime.InteropServices.PreserveSig]
			int GetImageRect(
				int i,
				ref RECT prc);

			[System.Runtime.InteropServices.PreserveSig]
			int GetIconSize(
				ref int cx,
				ref int cy);

			[System.Runtime.InteropServices.PreserveSig]
			int SetIconSize(
				int cx,
				int cy);

			[System.Runtime.InteropServices.PreserveSig]
			int GetImageCount(
				ref int pi);

			[System.Runtime.InteropServices.PreserveSig]
			int SetImageCount(
				int uNewCount);

			[System.Runtime.InteropServices.PreserveSig]
			int SetBkColor(
				int clrBk,
				ref int pclr);

			[System.Runtime.InteropServices.PreserveSig]
			int GetBkColor(
				ref int pclr);

			[System.Runtime.InteropServices.PreserveSig]
			int BeginDrag(
				int iTrack,
				int dxHotspot,
				int dyHotspot);

			[System.Runtime.InteropServices.PreserveSig]
			int EndDrag();

			[System.Runtime.InteropServices.PreserveSig]
			int DragEnter(
				System.IntPtr hwndLock,
				int x,
				int y);

			[System.Runtime.InteropServices.PreserveSig]
			int DragLeave(
				System.IntPtr hwndLock);

			[System.Runtime.InteropServices.PreserveSig]
			int DragMove(
				int x,
				int y);

			[System.Runtime.InteropServices.PreserveSig]
			int SetDragCursorImage(
				ref IImageList punk,
				int iDrag,
				int dxHotspot,
				int dyHotspot);

			[System.Runtime.InteropServices.PreserveSig]
			int DragShowNolock(
				int fShow);

			[System.Runtime.InteropServices.PreserveSig]
			int GetDragImage(
				ref POINT ppt,
				ref POINT pptHotspot,
				ref System.Guid riid,
				ref System.IntPtr ppv);

			[System.Runtime.InteropServices.PreserveSig]
			int GetItemFlags(
				int i,
				ref int dwFlags);

			[System.Runtime.InteropServices.PreserveSig]
			int GetOverlayImage(
				int iOverlay,
				ref int piIndex);
		} ;

		#endregion

		#endregion

		#region Properties

		/// <summary>
		/// Gets the hImageList handle
		/// </summary>
		public System.IntPtr Handle
		{
			get { return this.hIml; }
		}

		/// <summary>
		/// Gets/sets the size of System Image List to retrieve.
		/// </summary>
		public SystemImageListSize ImageListSize
		{
			get { return size; }
			set
			{
				size = value;
				create();
			}
		}

		/// <summary>
		/// Returns the size of the Image List Icons.
		/// </summary>
		public System.Drawing.Size Size
		{
			get
			{
				int cx = 0;
				int cy = 0;

				if (iImageList == null)
					ImageList_GetIconSize(hIml, ref cx, ref cy);
				else
					iImageList.GetIconSize(ref cx, ref cy);

				System.Drawing.Size sz = new System.Drawing.Size(cx, cy);

				return sz;
			}
		}

		#endregion

		#region Methods

		/// <summary>
		/// Returns a GDI+ copy of the icon from the ImageList
		/// at the specified index.
		/// </summary>
		/// <param name="index">The index to get the icon for</param>
		/// <returns>The specified icon</returns>
		public System.Drawing.Icon Icon(int index)
		{
			System.Drawing.Icon icon = null;

			System.IntPtr hIcon = System.IntPtr.Zero;
			if (iImageList == null)
			{
				hIcon = ImageList_GetIcon(
					hIml,
					index,
					(int) ImageListDrawItemConstants.ILD_TRANSPARENT);

			}
			else
			{
				iImageList.GetIcon(
					index,
					(int) ImageListDrawItemConstants.ILD_TRANSPARENT,
					ref hIcon);
			}

			if (hIcon != System.IntPtr.Zero)
			{
				icon = System.Drawing.Icon.FromHandle(hIcon);
			}
			return icon;
		}

		/// <summary>
		/// Return the index of the icon for the specified file, always using 
		/// the cached version where possible.
		/// </summary>
		/// <param name="fileName">Filename to get icon for</param>
		/// <returns>Index of the icon</returns>
		public int IconIndex(string fileName)
		{
			return IconIndex(fileName, false);
		}

		/// <summary>
		/// Returns the index of the icon for the specified file
		/// </summary>
		/// <param name="fileName">Filename to get icon for</param>
		/// <param name="forceLoadFromDisk">If True, then hit the disk to get the icon,
		/// otherwise only hit the disk if no cached icon is available.</param>
		/// <returns>Index of the icon</returns>
		public int IconIndex(
			string fileName,
			bool forceLoadFromDisk)
		{
			return IconIndex(
				fileName,
				forceLoadFromDisk,
				ShellIconStateConstants.ShellIconStateNormal);
		}

		/// <summary>
		/// Returns the index of the icon for the specified file
		/// </summary>
		/// <param name="fileName">Filename to get icon for</param>
		/// <param name="forceLoadFromDisk">If True, then hit the disk to get the icon,
		/// otherwise only hit the disk if no cached icon is available.</param>
		/// <param name="iconState">Flags specifying the state of the icon
		/// returned.</param>
		/// <returns>Index of the icon</returns>
		public int IconIndex(
			string fileName,
			bool forceLoadFromDisk,
			ShellIconStateConstants iconState
			)
		{
			SHGetFileInfoConstants dwFlags = SHGetFileInfoConstants.SHGFI_SYSICONINDEX;
			int dwAttr = 0;
			if (size == SystemImageListSize.SmallIcons)
			{
				dwFlags |= SHGetFileInfoConstants.SHGFI_SMALLICON;
			}

			// We can choose whether to access the disk or not. If you don't
			// hit the disk, you may get the wrong icon if the icon is
			// not cached. Also only works for files.
			if (!forceLoadFromDisk)
			{
				dwFlags |= SHGetFileInfoConstants.SHGFI_USEFILEATTRIBUTES;
				dwAttr = FILE_ATTRIBUTE_NORMAL;
			}
			else
			{
				dwAttr = 0;
			}

			// sFileSpec can be any file. You can specify a
			// file that does not exist and still get the
			// icon, for example sFileSpec = "C:\PANTS.DOC"
			SHFILEINFO shfi = new SHFILEINFO();
			uint shfiSize = (uint) System.Runtime.InteropServices.Marshal.SizeOf(shfi.GetType());
			System.IntPtr retVal = SHGetFileInfo(
				fileName, dwAttr, ref shfi, shfiSize,
				((uint) (dwFlags) | (uint) iconState));

			if (retVal.Equals(System.IntPtr.Zero))
			{
				//System.Diagnostics.Debug.Assert((!retVal.Equals(IntPtr.Zero)),"Failed to get icon index");
				return 0;
			}
			else
			{
				return shfi.iIcon;
			}
		}

		/// <summary>
		/// Draws an image
		/// </summary>
		/// <param name="hdc">Device context to draw to</param>
		/// <param name="index">Index of image to draw</param>
		/// <param name="x">X Position to draw at</param>
		/// <param name="y">Y Position to draw at</param>
		public void DrawImage(
			System.IntPtr hdc,
			int index,
			int x,
			int y
			)
		{
			DrawImage(hdc, index, x, y, ImageListDrawItemConstants.ILD_TRANSPARENT);
		}

		/// <summary>
		/// Draws an image using the specified flags
		/// </summary>
		/// <param name="hdc">Device context to draw to</param>
		/// <param name="index">Index of image to draw</param>
		/// <param name="x">X Position to draw at</param>
		/// <param name="y">Y Position to draw at</param>
		/// <param name="flags">Drawing flags</param>
		public void DrawImage(
			System.IntPtr hdc,
			int index,
			int x,
			int y,
			ImageListDrawItemConstants flags
			)
		{
			if (iImageList == null)
			{
				int ret = ImageList_Draw(
					hIml,
					index,
					hdc,
					x,
					y,
					(int) flags);
			}
			else
			{
				IMAGELISTDRAWPARAMS pimldp = new IMAGELISTDRAWPARAMS();
				pimldp.hdcDst = hdc;
				pimldp.cbSize = System.Runtime.InteropServices.Marshal.SizeOf(pimldp.GetType());
				pimldp.i = index;
				pimldp.x = x;
				pimldp.y = y;
				pimldp.rgbFg = -1;
				pimldp.fStyle = (int) flags;
				iImageList.Draw(ref pimldp);
			}

		}

		/// <summary>
		/// Draws an image using the specified flags and specifies
		/// the size to clip to (or to stretch to if ILD_SCALE
		/// is provided).
		/// </summary>
		/// <param name="hdc">Device context to draw to</param>
		/// <param name="index">Index of image to draw</param>
		/// <param name="x">X Position to draw at</param>
		/// <param name="y">Y Position to draw at</param>
		/// <param name="flags">Drawing flags</param>
		/// <param name="cx">Width to draw</param>
		/// <param name="cy">Height to draw</param>
		public void DrawImage(
			System.IntPtr hdc,
			int index,
			int x,
			int y,
			ImageListDrawItemConstants flags,
			int cx,
			int cy
			)
		{
			IMAGELISTDRAWPARAMS pimldp = new IMAGELISTDRAWPARAMS();
			pimldp.hdcDst = hdc;
			pimldp.cbSize = System.Runtime.InteropServices.Marshal.SizeOf(pimldp.GetType());
			pimldp.i = index;
			pimldp.x = x;
			pimldp.y = y;
			pimldp.cx = cx;
			pimldp.cy = cy;
			pimldp.fStyle = (int) flags;
			if (iImageList == null)
			{
				pimldp.himl = hIml;
				int ret = ImageList_DrawIndirect(ref pimldp);
			}
			else
			{
				iImageList.Draw(ref pimldp);
			}
		}

		/// <summary>
		/// Draws an image using the specified flags and state on XP systems.
		/// </summary>
		/// <param name="hdc">Device context to draw to</param>
		/// <param name="index">Index of image to draw</param>
		/// <param name="x">X Position to draw at</param>
		/// <param name="y">Y Position to draw at</param>
		/// <param name="flags">Drawing flags</param>
		/// <param name="cx">Width to draw</param>
		/// <param name="cy">Height to draw</param>
		/// <param name="foreColor">Fore colour to blend with when using the 
		/// ILD_SELECTED or ILD_BLEND25 flags</param>
		/// <param name="stateFlags">State flags</param>
		/// <param name="glowOrShadowColor">If stateFlags include ILS_GLOW, then
		/// the colour to use for the glow effect.  Otherwise if stateFlags includes 
		/// ILS_SHADOW, then the colour to use for the shadow.</param>
		/// <param name="saturateColorOrAlpha">If stateFlags includes ILS_ALPHA,
		/// then the alpha component is applied to the icon. Otherwise if 
		/// ILS_SATURATE is included, then the (R,G,B) components are used
		/// to saturate the image.</param>
		public void DrawImage(
			System.IntPtr hdc,
			int index,
			int x,
			int y,
			ImageListDrawItemConstants flags,
			int cx,
			int cy,
			System.Drawing.Color foreColor,
			ImageListDrawStateConstants stateFlags,
			System.Drawing.Color saturateColorOrAlpha,
			System.Drawing.Color glowOrShadowColor
			)
		{
			IMAGELISTDRAWPARAMS pimldp = new IMAGELISTDRAWPARAMS();
			pimldp.hdcDst = hdc;
			pimldp.cbSize = System.Runtime.InteropServices.Marshal.SizeOf(pimldp.GetType());
			pimldp.i = index;
			pimldp.x = x;
			pimldp.y = y;
			pimldp.cx = cx;
			pimldp.cy = cy;
			pimldp.rgbFg = System.Drawing.Color.FromArgb(0,
			                                             foreColor.R, foreColor.G, foreColor.B).ToArgb();
			System.Console.WriteLine("{0}", pimldp.rgbFg);
			pimldp.fStyle = (int) flags;
			pimldp.fState = (int) stateFlags;
			if ((stateFlags & ImageListDrawStateConstants.ILS_ALPHA) ==
				ImageListDrawStateConstants.ILS_ALPHA)
			{
				// Set the alpha:
				pimldp.Frame = (int) saturateColorOrAlpha.A;
			}
			else if ((stateFlags & ImageListDrawStateConstants.ILS_SATURATE) ==
				ImageListDrawStateConstants.ILS_SATURATE)
			{
				// discard alpha channel:
				saturateColorOrAlpha = System.Drawing.Color.FromArgb(0,
				                                                     saturateColorOrAlpha.R,
				                                                     saturateColorOrAlpha.G,
				                                                     saturateColorOrAlpha.B);
				// set the saturate color
				pimldp.Frame = saturateColorOrAlpha.ToArgb();
			}
			glowOrShadowColor = System.Drawing.Color.FromArgb(0,
			                                                  glowOrShadowColor.R,
			                                                  glowOrShadowColor.G,
			                                                  glowOrShadowColor.B);
			pimldp.crEffect = glowOrShadowColor.ToArgb();
			if (iImageList == null)
			{
				pimldp.himl = hIml;
				int ret = ImageList_DrawIndirect(ref pimldp);
			}
			else
			{
				iImageList.Draw(ref pimldp);
			}
		}

		#endregion				
	}

	#endregion

	#region SystemImageListHelper class

	/// <summary>
	/// Helper Methods for Connecting SystemImageList to Common Controls
	/// </summary>
	public class SystemImageListHelper
	{
		#region Constants

		private const int LVM_FIRST = 0x1000;
		private const int LVM_SETIMAGELIST = (LVM_FIRST + 3);

		private const int LVSIL_NORMAL = 0;
		private const int LVSIL_SMALL = 1;
		private const int LVSIL_STATE = 2;

		private const int TV_FIRST = 0x1100;
		private const int TVM_SETIMAGELIST = (TV_FIRST + 9);

		private const int TVSIL_NORMAL = 0;
		private const int TVSIL_STATE = 2;

		#endregion

		#region Implementation

		[System.Runtime.InteropServices.DllImport("user32", CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		private static extern System.IntPtr SendMessage(
			System.IntPtr hWnd,
			int wMsg,
			System.IntPtr wParam,
			System.IntPtr lParam);

		#endregion

		#region Methods

		/// <summary>
		/// Associates a SysImageList with a ListView control
		/// </summary>
		/// <param name="listView">ListView control to associate ImageList with</param>
		/// <param name="sysImageList">System Image List to associate</param>
		/// <param name="forStateImages">Whether to add ImageList as StateImageList</param>
		public static void SetListViewImageList(
			System.Windows.Forms.ListView listView,
			SystemImageList sysImageList,
			bool forStateImages
			)
		{
			System.IntPtr wParam = (System.IntPtr) LVSIL_NORMAL;
			if (sysImageList.ImageListSize == SystemImageListSize.SmallIcons)
			{
				wParam = (System.IntPtr) LVSIL_SMALL;
			}
			if (forStateImages)
			{
				wParam = (System.IntPtr) LVSIL_STATE;
			}
			SendMessage(
				listView.Handle,
				LVM_SETIMAGELIST,
				wParam,
				sysImageList.Handle);
		}

		/// <summary>
		/// Associates a SysImageList with a TreeView control
		/// </summary>
		/// <param name="treeView">TreeView control to associated ImageList with</param>
		/// <param name="sysImageList">System Image List to associate</param>
		/// <param name="forStateImages">Whether to add ImageList as StateImageList</param>
		public static void SetTreeViewImageList(
			System.Windows.Forms.TreeView treeView,
			SystemImageList sysImageList,
			bool forStateImages
			)
		{
			System.IntPtr wParam = (System.IntPtr) TVSIL_NORMAL;
			if (forStateImages)
			{
				wParam = (System.IntPtr) TVSIL_STATE;
			}
			SendMessage(
				treeView.Handle,
				TVM_SETIMAGELIST,
				wParam,
				sysImageList.Handle);
		}

		#endregion
	}

	#endregion
}