/*
 * Java wrapper for Jarvis NodeIterator
 */

package jarvis;

public class NodeIterator {
    private long jarvisHandle;
    private Node current;

    private NodeIterator(long handle, Node origin)
    {
        jarvisHandle = handle;
        current = origin;
    }

    public boolean done() //replaces the override of bool()
        { return current == null; }

    public native void next();

    //    public native NodeIterator filter(PropertyPredicate pp);
    //    public native void process();



    private void null_check(String method) throws Exception
    {
        if (current == null)
            throw new Exception(2, "null_iterator",
                                "NodeIterator." + method, null);
    }

    // Wrap the functions for node, so we can act on the current
    // Because java doesn't allow overriding operators, we cannot
    //  just let them slide through to jarvis.
    public Node get_current() throws Exception
    {
        null_check("get_current");
        return current;
    }

    public StringID get_tag() throws Exception
    {
        null_check("get_tag");
        return current.get_tag();
    }

    public Property get_property(StringID prop_id) throws Exception
    {
        null_check("get_property");
        return current.get_property(prop_id);
    }

    public PropertyIterator get_properties() throws Exception
    {
        null_check("get_properties");
        return current.get_properties();
    }

    public void set_property(StringID prop_id, Property prop) throws Exception
    {
        null_check("set_property");
        current.set_property(prop_id, prop);
    }

    public void remove_property(StringID prop_id) throws Exception
    {
        null_check("remove_property");
        current.remove_property(prop_id);
    }

    public void finalize() { dispose(); }
    public native void dispose();
}
