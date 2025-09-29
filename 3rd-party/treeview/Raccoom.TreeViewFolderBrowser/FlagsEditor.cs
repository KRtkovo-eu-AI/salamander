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

namespace Raccoom.Windows.Forms.Design
{
    /// <summary>
    /// Implements a custom type editor for enum's with FlagAttribute
    /// </summary>
    /// <remarks>
    /// Copyright by Thierry Bouquain, <a href="http://www.codeproject.com/cs/miscctrl/flagseditor.asp?target=FlagsEditor" target="_blank">A flag editor article on codeproject.com</a>
    /// </remarks>    
    [System.Security.Permissions.PermissionSet(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
    public sealed class EnumEditor : System.Drawing.Design.UITypeEditor, System.IDisposable
    {
        #region fields
        private bool handleLostfocus;
        private System.Windows.Forms.Design.IWindowsFormsEditorService edSvc;
        private System.Windows.Forms.CheckedListBox clb;
        private System.Windows.Forms.ToolTip tooltipControl; 
        #endregion

        #region public interface
        /// <summary>
        /// Overrides the method used to provide basic behaviour for selecting editor.
        /// Shows our custom control for editing the value.
        /// </summary>
        /// <param name="context">The context of the editing control</param>
        /// <param name="provider">A valid service provider</param>
        /// <param name="value">The current value of the object to edit</param>
        /// <returns>The new value of the object</returns>
        public override object EditValue(System.ComponentModel.ITypeDescriptorContext context, System.IServiceProvider provider, object value)
        {
            if (context != null
                && context.Instance != null
                && provider != null)
            {
                edSvc = (System.Windows.Forms.Design.IWindowsFormsEditorService)provider.GetService(typeof(System.Windows.Forms.Design.IWindowsFormsEditorService));

                if (edSvc != null)
                {
                    // Create a CheckedListBox and populate it with all the enum values
                    clb = new System.Windows.Forms.CheckedListBox();
                    clb.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
                    clb.CheckOnClick = true;
                    clb.MouseDown += new System.Windows.Forms.MouseEventHandler(this.OnMouseDown);
                    clb.MouseMove += new System.Windows.Forms.MouseEventHandler(this.OnMouseMoved);

                    tooltipControl = new System.Windows.Forms.ToolTip();
                    tooltipControl.ShowAlways = true;

                    foreach (string name in System.Enum.GetNames(context.PropertyDescriptor.PropertyType))
                    {
                        // Get the enum value
                        object enumVal = System.Enum.Parse(context.PropertyDescriptor.PropertyType, name);
                        // Get the int value 
                        int intVal = (int)System.Convert.ChangeType(enumVal, typeof(int), System.Globalization.CultureInfo.CurrentCulture);

                        // Get the description attribute for this field
                        System.Reflection.FieldInfo fi = context.PropertyDescriptor.PropertyType.GetField(name);
                        System.ComponentModel.DescriptionAttribute[] attrs = (System.ComponentModel.DescriptionAttribute[])fi.GetCustomAttributes(typeof(System.ComponentModel.DescriptionAttribute), false);

                        // Store the the description
                        string tooltip = attrs.Length > 0 ? attrs[0].Description : string.Empty;

                        // Get the int value of the current enum value (the one being edited)
                        int intEdited = (int)System.Convert.ChangeType(value, typeof(int), System.Globalization.CultureInfo.CurrentCulture);

                        // Creates a clbItem that stores the name, the int value and the tooltip
                        EnumEditorItem item = new EnumEditorItem(enumVal.ToString(), intVal, tooltip);

                        // Get the checkstate from the value being edited
                        //bool checkedItem = (intEdited & intVal) > 0;
                        bool checkedItem = (intEdited & intVal) == intVal;

                        // Add the item with the right check state
                        clb.Items.Add(item, checkedItem);
                    }

                    // Show our CheckedListbox as a DropDownControl. 
                    // This methods returns only when the dropdowncontrol is closed
                    edSvc.DropDownControl(clb);

                    // Get the sum of all checked flags
                    int result = 0;
                    foreach (EnumEditorItem obj in clb.CheckedItems)
                    {
                        //result += obj.Value;
                        result |= obj.Value;
                    }

                    // return the right enum value corresponding to the result
                    return System.Enum.ToObject(context.PropertyDescriptor.PropertyType, result);
                }
            }

            return value;
        }

        /// <summary>
        /// Shows a dropdown icon in the property editor
        /// </summary>
        /// <param name="context">The context of the editing control</param>
        /// <returns>Returns <c>UITypeEditorEditStyle.DropDown</c></returns>
        public override System.Drawing.Design.UITypeEditorEditStyle GetEditStyle(System.ComponentModel.ITypeDescriptorContext context)
        {
            return System.Drawing.Design.UITypeEditorEditStyle.DropDown;
        }
        
        #endregion

        #region internal interface
        /// <summary>
        /// When got the focus, handle the lost focus event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnMouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (!handleLostfocus && clb.ClientRectangle.Contains(clb.PointToClient(new System.Drawing.Point(e.X, e.Y))))
            {
                clb.LostFocus += new System.EventHandler(this.ValueChanged);
                handleLostfocus = true;
            }
        }

        /// <summary>
        /// Occurs when the mouse is moved over the checkedlistbox. 
        /// Sets the tooltip of the item under the pointer
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnMouseMoved(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            int index = clb.IndexFromPoint(e.X, e.Y);
            if (index >= 0)
                tooltipControl.SetToolTip(clb, ((EnumEditorItem)clb.Items[index]).Tooltip);
        }

        /// <summary>
        /// Close the dropdowncontrol when the user has selected a value
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ValueChanged(object sender, System.EventArgs e)
        {
            if (edSvc != null)
            {
                edSvc.CloseDropDown();
            }
        } 
        #endregion

        #region nested types
        /// <summary>
        /// Internal class used for storing custom data in listviewitems
        /// </summary>
        internal struct EnumEditorItem
        {
            /// <summary>
            /// Creates a new instance of the <c>clbItem</c>
            /// </summary>
            /// <param name="str">The string to display in the <c>ToString</c> method. 
            /// It will contains the name of the flag</param>
            /// <param name="value">The integer value of the flag</param>
            /// <param name="tooltip">The tooltip to display in the <see cref="System.Windows.Forms.CheckedListBox"/></param>
            public EnumEditorItem(string str, int value, string tooltip)
            {
                this.str = str;
                this.value = value;
                this.tooltip = tooltip;
            }

            private string str;

            private int value;

            /// <summary>
            /// Gets the int value for this item
            /// </summary>
            public int Value
            {
                get { return value; }
            }

            private string tooltip;

            /// <summary>
            /// Gets the tooltip for this item
            /// </summary>
            public string Tooltip
            {
                get { return tooltip; }
            }

            /// <summary>
            /// Gets the name of this item
            /// </summary>
            /// <returns>The name passed in the constructor</returns>
            public override string ToString()
            {
                return str;
            }
        }
        #endregion

        #region IDisposable Members
        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources. 
        /// </summary>
        public void Dispose()
        {
            this.clb.Dispose();
            this.tooltipControl.Dispose();
        }



        #endregion
    }
}