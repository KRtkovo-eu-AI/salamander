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

namespace Raccoom.Windows.Forms
{
	/// <summary>
	/// Defines the DriveTypes used for Win32_LogicalDisk<seealso cref="TreeViewFolderBrowser"/>.This enumeration can be treated as a bit field, that is, a set of flags.
	/// </summary>
	[System.Flags]
	[System.ComponentModel.Editor(typeof (Raccoom.Windows.Forms.Design.EnumEditor), typeof (System.Drawing.Design.UITypeEditor))]
	public enum DriveTypes
	{
		/// <summary>All drive types</summary>
		All = NoRootDirectory | RemovableDisk | LocalDisk | NetworkDrive | CompactDisc | RAMDisk,
		/// <summary>
		/// NoRootDirectory
		/// </summary>
		NoRootDirectory = 0x0001,
		/// <summary>
		/// Drive has removable media. This includes all floppy drives and many other varieties of storage devices.
		/// </summary>
		RemovableDisk = 0x0002,
		/// <summary>
		/// Drive has fixed (nonremovable) media. This includes all hard drives, including hard drives that are removable.
		/// </summary>
		LocalDisk = 0x0004,
		/// <summary>
		/// Network drives. This includes drives shared anywhere on a network.
		/// </summary>
		NetworkDrive = 0x0008,
		/// <summary>
		/// Drive is a CD-ROM. No distinction is made between read-only and read/write CD-ROM drives.
		/// </summary>
		CompactDisc = 0x0020,
		/// <summary>
		/// Drive is a block of Random Access Memory (RAM) on the local computer that behaves like a disk drive.
		/// </summary>
		RAMDisk = 0x0040
	}

	/// <summary>
	/// Defines the DriveTypes used for Win32_LogicalDisk<seealso cref="TreeViewFolderBrowser"/>.This enumeration can a<b>not</b> be treated as a bit field
	/// </summary>
	public enum Win32_LogicalDiskDriveTypes
	{
		/// <summary>
		/// NoRootDirectory
		/// </summary>
		NoRootDirectory = 1,
		/// <summary>
		/// Drive has removable media. This includes all floppy drives and many other varieties of storage devices.
		/// </summary>
		RemovableDisk,
		/// <summary>
		/// Drive has fixed (nonremovable) media. This includes all hard drives, including hard drives that are removable.
		/// </summary>
		LocalDisk,
		/// <summary>
		/// Network drives. This includes drives shared anywhere on a network.
		/// </summary>
		NetworkDrive,
		/// <summary>
		/// Drive is a CD-ROM. No distinction is made between read-only and read/write CD-ROM drives.
		/// </summary>
		CompactDisc,
		/// <summary>
		/// Drive is a block of Random Access Memory (RAM) on the local computer that behaves like a disk drive.
		/// </summary>
		RAMDisk
	}
}